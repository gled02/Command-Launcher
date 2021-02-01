//  Partie implantation du module thread_pool

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "MACROS.h"
#include "thread_pool.h"

#define ARG_SEPARATOR " "
#define MAX_ARG_COUNT 200
#define MAX_ARG_LENGTH 200

#define FUN_SUCCESS      0
#define FUN_FAILURE     -1

// Structure qui permet de stocker un thread avec les informations nécessaires.
typedef struct thread_one {
  pthread_t thread;
  pthread_mutex_t mutex;
  size_t connection;
  size_t shm_size;
  void *data;
  volatile bool end;
} thread_one;

// Structure principale stockant les threads
struct threads {
  thread_one **array;
  size_t count;
  size_t min_thread;
  size_t max_thread;
  size_t max_con;
  size_t shm_size;
};

//  ini__thread_one_element: fonction statique qui permet d'allouer et
//  initialiser les champs de la structure thread_one. thread_one sera un
//  élément de la structure threads. Renvoie NULL en cas de dépassement de
// capacité ou d'erreur, un pointeur vers l'objet en cas de succès.
static thread_one *ini__thread_one_element(size_t number, size_t max_con,
    size_t shm_size);

//  waiting_command: fonction statique exécuté par chaque thread les mettant
//  en phase d'attente pour une écriture dans la shm. Renvoie NULL toujours.
static void *waiting_command(void *arg);

//  fork_thread: fonction statique qui permet à un thread d'exécuter une
//  commande lu dans sa shm. L'exécution est un appel à execv dans un fork et
//  le résultat est stocké dans le paramètre command. Renvoie 0 en cas de
//  succès, -1 en cas d'échec.
static int fork_thread(char *command, size_t size);

threads *ini_threads(size_t min_thread, size_t max_con, size_t max_thread,
    size_t shm_size) {
  threads *ptr = malloc(sizeof(threads));
  if (ptr == NULL) {
    return NULL;
  }
  ptr->count = min_thread;
  ptr->max_con = max_con;
  ptr->min_thread = min_thread;
  ptr->max_thread = max_thread;
  ptr->shm_size = shm_size;
  ptr->array = calloc(1, sizeof(thread_one *) * max_thread);
  if (ptr->array == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < min_thread; ++i) {
    ptr->array[i] = ini__thread_one_element(i, max_con, shm_size);
    if (ptr->array[i] == NULL) {
      return NULL;
    }
  }
  return ptr;
}

int connected_thread_to(threads *th) {
  for (size_t i = 0; i < th->count; ++i) {
    // On verifie s'il y a des threads libres
    if (th->array[i] != NULL
        && pthread_mutex_trylock(&(th->array[i]->mutex)) == 0) {
      // Un thread a été trouvé
      return (int) i;
    }
  }
  // Pas de thread disponible, on regarde si on a atteint le max
  if (th->count < th->max_thread) {
    // Si on a de place, on créé un nouveau thread
    th->array[th->count] = ini__thread_one_element(th->count, th->max_con,
            th->shm_size);
    ++th->count;
    // On renvoie le nouveau thread
    pthread_mutex_lock(&(th->array[th->count - 1]->mutex));
    return (int) th->count - 1;
  }
  return FUN_FAILURE;
}

int used_thread(threads *th, size_t number) {
  // On regarde si le thread à atteint sa fin de vie
  if (th->array[number]->connection == 1) {
    // On indique au thread qu'il s'arrête totalement
    th->array[number]->end = true;
    if (pthread_join(th->array[number]->thread, NULL) != 0) {
      perror("used_thread: pthread_join");
      return FUN_FAILURE;
    }
    // Libération de la shm
    char shm_name[SHM_LENGTH];
    sprintf(shm_name, "%s%zu", SHM_NAME, number);
    if (shm_unlink(shm_name) == -1) {
      perror("used_thread: shm_unlink");
      return FUN_FAILURE;
    }
    free(th->array[number]);
    th->array[number] = NULL;
    if (th->count == th->min_thread) {
      // Réinitialisation du thread
      th->array[number] = ini__thread_one_element(number, th->max_con,
              th->shm_size);
      return 1;
    }
    // On renvoit une autre valeur pour prévenir un changement d'état
    return 2;
  }
  if (th->array[number]->connection > 1) {
    th->array[number]->connection -= 1;
  }
  pthread_mutex_unlock(&(th->array[number]->mutex));
  return FUN_SUCCESS;
}

void thread_dispose(threads *th) {
  for (size_t i = 0; i < th->count; ++i) {
    if (th->array[i] != NULL) {
      // On arrête le thread
      th->array[i]->end = true;
      pthread_join(th->array[i]->thread, NULL);
      free(th->array[i]);
      th->array[i] = NULL;
    }
  }
  free(th->array);
  free(th);
  th = NULL;
}

thread_one *ini__thread_one_element(size_t number, size_t max_con,
    size_t shm_size) {
  thread_one *ptr = malloc(sizeof(thread_one));
  if (ptr == NULL) {
    return NULL;
  }
  if (pthread_mutex_init(&(ptr->mutex), NULL) != 0) {
    perror("ini__thread_one_element: pthread_mutex_init");
    return NULL;
  }
  ptr->connection = max_con;
  ptr->shm_size = shm_size;
  char shmname[SHM_LENGTH];
  sprintf(shmname, "%s%ld", SHM_NAME, number);
  int shm_fd = shm_open(shmname, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (shm_fd == -1) {
    perror("ini__thread_one_element: shm_open");
    return NULL;
  }
  if (ftruncate(shm_fd, (long int) shm_size) == -1) {
    perror("ini__thread_one_element: ftruncate");
    return NULL;
  }
  ptr->data = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED,
          shm_fd, 0);
  if (ptr->data == MAP_FAILED) {
    perror("ini__thread_one_element: mmap");
    return NULL;
  }
  if (close(shm_fd) == -1) {
    perror("ini__thread_one_element: close shm");
    return NULL;
  }
  ptr->end = false;
  int *flag = (int *) ptr->data;
  *flag = SHM_FLAG;
  pthread_create(&(ptr->thread), NULL, waiting_command, ptr);
  return ptr;
}

void *waiting_command(void *arg) {
  thread_one *ptr = (thread_one *) arg;
  while (!ptr->end) {
    // Lecture d'une commande
    volatile int *flag = (volatile int *) ptr->data;
    char *data = (char *) ptr->data + sizeof *flag;
    // En attente d'une ressource destiné au thread ou d'une demande
    //  d'arrêt immédiat.
    while (*flag != COMMANDE_FLAG && !ptr->end) {
    }
    // Si c'est un arrêt immédiat, on s'arrête.
    if (ptr->end) {
      return NULL;
    }
    // Execution de la commande, on enleve 4 octets car il faut compter
    // la taille du flag dans la shm
    if (fork_thread(data, ptr->shm_size - sizeof(int))
        == FUN_FAILURE) {
      return NULL;
    }
    *flag = RESULT_FLAG;
  }
  return NULL;
}

int fork_thread(char *command, size_t size) {
  // On enlève 1 de size pour pouvoir insérer le caractère de fin de
  // chaine dans le cas ou toute la taille est utilisée.
  size = size - 1;
  // Split des arguments dans un tableau de tableau de caractère
  char *arr[MAX_ARG_COUNT];
  size_t i = 0;
  char *token = strtok(command, ARG_SEPARATOR);
  while (token != NULL) {
    arr[i] = token;
    ++i;
    token = strtok(NULL, ARG_SEPARATOR);
  }
  arr[i] = NULL;
  int p[2];
  if (pipe(p) == -1) {
    perror("fork_thread: pipe");
    return FUN_FAILURE;
  }
  switch (fork()) {
    case -1:
      perror("fork_thread: fork");
      return FUN_FAILURE;
    case 0:
      if (close(p[0]) == -1) {
        perror("fork_thread: close fils");
        return FUN_FAILURE;
      }
      if (dup2(p[1], STDOUT_FILENO) == -1) {
        perror("fork_thread: dup2 stdout");
        return FUN_FAILURE;
      }
      if (dup2(p[1], STDERR_FILENO) == -1) {
        perror("fork_thread: dup2 stderr");
        return FUN_FAILURE;
      }
      execv(arr[0], arr);
      printf("Votre commande est invalide.\n");
      exit(EXIT_SUCCESS);
      break;
    default:
      if (close(p[1]) == -1) {
        return FUN_FAILURE;
      }
      ssize_t r = 0;
      if ((r = read(p[0], command, size)) == -1) {
        perror("fork_thread: read");
        return FUN_SUCCESS;
      }
      command[r] = '\0';
      break;
  }
  return FUN_SUCCESS;
}
