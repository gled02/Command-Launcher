//  Partie implantation du module client

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "client.h"
#include "MACROS.h"

#define NO_THREAD     -2
#define SHM_SIZE      30

#define FUN_SUCCESS   0
#define FUN_FAILURE   -1

//  Renvoie le nombre des chiffres d'un naturel.
static int digit__count(int n) {
  if (n < 10) {
    return 1;
  }
  return 1 + digit__count(n / 10);
}

int start_end_connection(int fd_tube, size_t label, char *msg) {
  char msg1[(int) strlen(msg) + digit__count((int) label) + 1];
  sprintf(msg1, "%s%zu", msg, label);
  if (write(fd_tube, msg1, sizeof msg1) == -1) {
    perror("start_end_connection: Impossible d'écrire dans le tude du demon");
    return FUN_FAILURE;
  }
  return FUN_SUCCESS;
}

int demon_answer(int fd_client, char *shm_name) {
  char answer[SHM_LENGTH];
  if (read(fd_client, answer, sizeof answer) == -1) {
    perror("demon_answer: Impossible de lire dans le tube du client");
    return FUN_FAILURE;
  }
  if (strcmp(answer, RST) == 0) {
    return NO_THREAD;
  }
  char size[SHM_SIZE];
  int i = 0;
  while (answer[i] <= '9') {
    size[i] = answer[i];
    ++i;
  }
  size[i] = '\0';
  int shm_size = atoi(size);
  if (shm_size == 0) {
    perror("demon_answer: Invalid taille de shm");
    return FUN_FAILURE;
  }
  strcpy(shm_name, answer + i - 1);
  if (close(fd_client) == -1) {
    perror("demon_answer: Impossible de fermer le tube de client");
    return FUN_FAILURE;
  }
  return shm_size;
}

int send_command_to_thread(char *shm_name, char *command, size_t shm_size) {
  int shm_fd;
  if ((shm_fd = shm_open(shm_name, O_RDWR | O_EXCL, S_IRUSR | S_IWUSR)) == -1) {
    perror("send_command_to_thread: Impossible d'ouvrir le shm");
    return FUN_FAILURE;
  }
  if (ftruncate(shm_fd, (long int) shm_size) == -1) {
    perror("send_command_to_thread: Impossible de projéter le shm");
    return FUN_FAILURE;
  }
  void *ptr
    = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr == MAP_FAILED) {
    perror("send_command_to_thread: Impossible de fixer la taille de shm");
    return FUN_FAILURE;
  }
  char *cmdptr = (char *) ptr + sizeof(int);
  char *endptr = cmdptr + strlen(command);
  // Copie de la commande a exécuter dans la shm
  // On exclu 4 octets à cause du int indiquant la nature de la donnée
  strncpy(cmdptr, command, shm_size - sizeof(int));
  // On indique la fin de la donnée
  *endptr = '\0';
  volatile int *flag = (int *) ptr;
  *flag = COMMANDE_FLAG;
  if (close(shm_fd) == -1) {
    perror("send_command_to_thread: Impossible de fermer le shm");
    return FUN_FAILURE;
  }
  return FUN_SUCCESS;
}

int receive_result_from_thread(char *shm_name, size_t shm_size) {
  int shm_fd;
  if ((shm_fd = shm_open(shm_name, O_RDWR | O_EXCL, S_IRUSR | S_IWUSR)) == -1) {
    perror("receive_result_from_thread: Impossible d'ouvrir le shm");
    return FUN_FAILURE;
  }
  if (ftruncate(shm_fd, (long int) shm_size) == -1) {
    perror("receive_result_from_thread: Impossible de projéter le shm");
    return FUN_FAILURE;
  }
  void *ptr
    = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr == MAP_FAILED) {
    perror("receive_result_from_thread: Impossible de fixer la taille de shm");
    return FUN_FAILURE;
  }
  volatile int *flag = (int *) ptr;
  while (*flag != RESULT_FLAG) {
  }
  char *result = (char *) ptr + sizeof(int);
  size_t i = 0;
  while (result[i] != '\0') {
    putchar(result[i]);
    ++i;
  }
  *flag = SHM_FLAG;
  result[0] = '\0';
  if (close(shm_fd) == -1) {
    perror("receive_result_from_thread: Impossible de fermer le shm");
    return FUN_FAILURE;
  }
  return FUN_SUCCESS;
}
