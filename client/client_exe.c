//  Partie implantation du module client_exe

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "client.h"
#include "MACROS.h"

#define FINISH  "exit"
#define START   "\nVous pouvez entrer votre commande (exit pour quitter): "

#define FUN_SUCCESS      0
#define FUN_FAILURE     -1

#define NO_THREAD     -2
#define BUFFER_SIZE   128

static int fd_demon;
static char shm_name[SHM_LENGTH];

// Envoit le message au démon, pour déconnecter le client.
static void send__end(int sig);

int main(void) {
  char tube_client[TUBE_NAME_LENGTH];
  pid_t pid = getpid();
  sprintf(tube_client, "%s%d", TUBE_CLIENT, pid);
  if (mkfifo(tube_client, S_IRUSR | S_IWUSR) == -1) {
    perror("client_exe: Impossible de créer le tube du client");
    exit(EXIT_FAILURE);
  }
  if ((fd_demon = open(TUBE_DEMON, O_WRONLY)) == -1) {
    perror("client_exe: Impossible d'ouvrir le tube du demon");
    exit(EXIT_FAILURE);
  }
  if (start_end_connection(fd_demon, (size_t) pid, SYNC) == FUN_FAILURE) {
    exit(EXIT_FAILURE);
  }
  int fd_client;
  if ((fd_client = open(tube_client, O_RDONLY)) == -1) {
    perror("client_exe: Impossible d'ouvrir le tube du client");
    exit(EXIT_FAILURE);
  }
  int shm_size = demon_answer(fd_client, shm_name);
  if (shm_size == FUN_FAILURE) {
    exit(EXIT_FAILURE);
  } else if (shm_size == NO_THREAD) {
    printf("client_exe: Impossible de se connecter, pas de thread disponible\n");
    exit(EXIT_SUCCESS);
  }
  char buffer[BUFFER_SIZE];
  signal(SIGINT, send__end);
  signal(SIGTERM, send__end);
  printf(START);
  while (fgets(buffer, BUFFER_SIZE, stdin) != NULL
      && strncmp(buffer, FINISH, strlen(FINISH)) != 0) {
    buffer[strlen(buffer) - 1] = '\0';
    if (send_command_to_thread(shm_name, buffer,
        (size_t) shm_size) == FUN_FAILURE) {
      exit(EXIT_FAILURE);
    }
    if (receive_result_from_thread(shm_name,
        (size_t) shm_size) == FUN_FAILURE) {
      exit(EXIT_FAILURE);
    }
    printf(START);
  }
  send__end(0);
  return EXIT_SUCCESS;
}

void send__end(int sig) {
  (void) sig;
  if (start_end_connection(fd_demon, (size_t) atoi(shm_name + strlen(SHM_NAME)),
      END) == FUN_FAILURE) {
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}
