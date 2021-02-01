//  Partie implantation du module demon

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include "thread_pool.h"
#include "MACROS.h"

#define CONFIG_LENGTH    4
#define THREAD_LENGTH    5
#define MSG_LENGTH       4

#define FUN_SUCCESS      0
#define FUN_FAILURE     -1

// Variable global de la structe du connected_thread de thread, elle est global
// pour qu'elle puisse être manipuler après un signal.
static threads *connected_thread;

//    connected_to_thread: permet de connecter le client avec un thread
// disponible. Si le nombre MAX_THREAD n’est pas atteint, alors le démon
// initialise un nouveau thread pour gérer la connexion et transmet au client
// les informations nécessaire pour se connecter à la SHM associée à ce nouveau
// thread. Si aucun thread n’est disponible et si le nombre MAX_THREAD est
// atteint, alors le démon répond négativement par l’envoi du message « RST ».
// Cette fonction est prévu pour ne jamais s'arrêter, seul les signaux peuvent
// y mettre fin. Renvoie -1 en cas d'erreur et 0 en cas de succès.
static int connected__to_thread(size_t shm_size);

//    dispose: libère proprement les ressources utilisées (données système et
// mémoire) et gère correctement les demandes de terminaisons via des signaux.
void dispose(int sig);

// Structure permettant de stocker les paramètres enregistrés dans le fichier
// demon.conf
typedef struct info {
  size_t min_thread;
  size_t max_thread;
  size_t max_connected_thread;
  size_t shm_size;
} info;

int main() {
  printf("Le fichier de configuration :\n\n");
  pid_t pid = fork();
  pid_t sid;
  if (pid < 0) {
    perror("demon: pid < 0");
    return EXIT_FAILURE;
  } else if (pid > 0) {
    return EXIT_SUCCESS;
  }
  sid = setsid();
  if (sid < 0) {
    perror("demon: Impossible de créer la session");
    return EXIT_FAILURE;
  }
  info *info_struct = malloc(sizeof(info));
  if (info_struct == NULL) {
    perror("demon: Erreur d'allocation pour info_struct");
    return EXIT_FAILURE;
  }
  FILE *f = fopen("demon.conf", "r");
  if (f == NULL) {
    perror("demon: Impossoble d'ouvrir le demon.conf");
    free(info_struct);
    return EXIT_FAILURE;
  }
  int count = 0;
  count += fscanf(f, "MIN_THREAD=%zu\n", &info_struct->min_thread);
  count += fscanf(f, "MAX_THREAD=%zu\n", &info_struct->max_thread);
  count += fscanf(f, "MAX_CONNECT_PER_THREAD=%zu\n",
          &info_struct->max_connected_thread);
  count += fscanf(f, "SHM_SIZE=%zu\n", &info_struct->shm_size);
  // Si le nombre de paramètres lu n'est pas le bon, on s'arrête
  if (count != CONFIG_LENGTH) {
    perror("demon: Impossible de lire le demon.conf");
    free(info_struct);
    return EXIT_FAILURE;
  } else {
    printf("min_thread : %zu\n", info_struct->min_thread);
    printf("max_thread : %zu\n", info_struct->max_thread);
    printf("max_connect_per_thread : %zu\n", info_struct->max_connected_thread);
    printf("shm_size : %zu\n\n", info_struct->shm_size);
  }
  if (fclose(f) == -1) {
    perror("demon: Impossible de fermer le demon.conf");
    free(info_struct);
    return EXIT_FAILURE;
  }
  // Initilisation de MIN_THREAD threads
  connected_thread = ini_threads(info_struct->min_thread,
          info_struct->max_connected_thread,
          info_struct->max_thread, info_struct->shm_size);
  if (connected_thread == NULL) {
    free(info_struct);
    return EXIT_FAILURE;
  }
  signal(SIGINT, dispose);
  signal(SIGTERM, dispose);
  if (connected__to_thread(info_struct->shm_size) == -1) {
    free(info_struct);
    return EXIT_FAILURE;
  }
  free(info_struct);
  dispose(0);
  return EXIT_SUCCESS;
}

int connected__to_thread(size_t shm_size) {
  int fd_client;
  char received_message[PID_LENGTH + MSG_LENGTH + 1];
  // Les messages envoyés seront de la forme RST ou
  // taille de la shmSHM_NAMEnumero du thread)
  char send_message[SHM_LENGTH] = {
    '0'
  };
  if (mkfifo(TUBE_DEMON, S_IRUSR | S_IWUSR) == -1) {
    perror("connected_to_thread: Impossible de créer le tube du demon");
    return FUN_FAILURE;
  }
  int fd_demon;
  if ((fd_demon = open(TUBE_DEMON, O_RDONLY)) == -1) {
    perror("connected_to_thread: Impossible d'ouvrir le tube du demon");
    return FUN_FAILURE;
  }
  while (1) {
    ssize_t n = 0;
    while ((n
          = read(fd_demon, received_message, sizeof received_message)) > 0) {
      char tube_client[strlen(TUBE_CLIENT) + PID_LENGTH + 1];
      strcpy(tube_client, TUBE_CLIENT);
      if (strncmp(SYNC, received_message, strlen(SYNC)) == 0) {
        printf("\nPid du processus: %s\n", received_message + strlen(SYNC));
        strcat(tube_client, received_message + strlen(SYNC));
        if ((fd_client = open(tube_client, O_WRONLY)) == -1) {
          perror("connected_to_thread: Impossible d'ouvrir le tube du client");
          return FUN_FAILURE;
        }
        if (unlink(tube_client) == -1) {
          perror("connected_to_thread: Impossible de unlink le tube client");
          return FUN_FAILURE;
        }
        int thread_number = connected_thread_to(connected_thread);
        if (thread_number == FUN_FAILURE) {
          if (write(fd_client, RST, sizeof RST) == -1) {
            perror(
              "connected_to_thread: Impossible d'écrire dans le tube du client");
            return FUN_FAILURE;
          }
          printf("RST: Pas de thread disponible! \n");
        } else {
          sprintf(send_message, "%zu%s%d", shm_size, SHM_NAME,
              thread_number);
          if (write(fd_client, send_message, sizeof send_message) == -1) {
            perror(
              "connected_to_thread: Impossible d'écrire dans le tube du client");
            return FUN_FAILURE;
          }
          printf("Le thread associé à vous: n°%d \n", thread_number);
        }
        if (close(fd_client) == -1) {
          perror("connected_to_thread: Impossible de fermer le tube du client");
          return FUN_FAILURE;
        }
      } else if (strncmp(END, received_message, strlen(END)) == 0) {
        char interrupted_thread_number[THREAD_LENGTH + 1];
        strcpy(interrupted_thread_number, received_message + strlen(END));
        int result = used_thread(connected_thread, (size_t) atoi(
                  interrupted_thread_number));
        if (result == FUN_FAILURE) {
          return FUN_FAILURE;
        }
        printf("Connection terminé dans le thread: n°%s\n",
            received_message + strlen(END));
        if (result == 1) {
          printf("Le thread réinitialisé: n°%s\n",
              received_message + strlen(END));
        } else if (result == 2) {
          printf("Le thread terminé: n°%s \n",
              received_message + strlen(END));
        }
      }
    }
  }
}

void dispose(int sig) {
  if (connected_thread != NULL) {
    thread_dispose(connected_thread);
  }
  unlink(TUBE_DEMON);
  sig = 0;
  int i = 0;
  char shm_name[SHM_LENGTH];
  do {
    sprintf(shm_name, "%s%d", SHM_NAME, i);
    ++i;
    sig = shm_unlink(shm_name);
  } while (sig != FUN_FAILURE);
  exit(EXIT_SUCCESS);
}
