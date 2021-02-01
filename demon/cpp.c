#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>

#define SIZE 128
#define DEFAULT_FLAGS O_WRONLY |O_CREAT

typedef struct arg arg;

struct arg {
  int flags;
  off_t off1;
  off_t off2;
  char* fichier_src;
  char* fichier_dest;
};

void init_args(arg *s);

int main(int argc, char** argv) {
  if (argc < 3) {
    perror("Incorrect number of arguments");
    return EXIT_FAILURE;
  }
  arg *args = malloc(sizeof(arg));
  if (args == NULL) {
    perror("Error malloc");
    return EXIT_FAILURE;
  }
  init_args(args);
  int opt;
  while ((opt = getopt(argc, argv, "vab:e:s:d:")) != -1) {
    switch (opt) {
      case 'v':
      args->flags = O_WRONLY | O_CREAT | O_EXCL;
      break;
      case 'a':
      args->flags = O_WRONLY | O_APPEND;
      break;
      case 'b':
      if ((args->off1 = atoi(optarg)) < 0) {
        perror("Offset must be positive");
        exit(EXIT_FAILURE);
      }
      break;
      case 'e':
      if ((args->off2 = atoi(optarg)) < 0) {
        perror("Offset must be positive");
        exit(EXIT_FAILURE);
      }
      break;
      case 's':
      args->fichier_src = optarg;
      break;
      case 'd':
      args->fichier_dest = optarg;
      break;
    }
  }
  if (args->fichier_src == NULL || args->fichier_dest == NULL) {
    perror("Donnez un fichier source et un fichier destinataire");
    exit(EXIT_FAILURE);
  }
  int fd_src = open(args->fichier_src, O_RDONLY);
  if (fd_src == -1) {
    perror("open fichier source");
    exit(EXIT_FAILURE);
  }
  int fd_dest = open(args->fichier_dest, args->flags, 0666);
  if (fd_dest == -1) {
    perror("open fichier destination");
    close(fd_src);
    exit(EXIT_FAILURE);
  }
  if (args->off1 != 0) {
    lseek(fd_src, args->off1, SEEK_SET);
  }
  long comp = 0;
  char *buf = malloc(SIZE);
  ssize_t n;
  while ((n = read(fd_src, buf, SIZE)) > 0) {
    if (write(fd_dest, buf, (size_t)n) < 0) {
      perror("write");
      free(buf);
      exit(EXIT_FAILURE);
    }
    comp = args->off1 + (long)n;
  }
  if (n < 0) {
    perror("read");
    free(buf);
    exit(EXIT_FAILURE);
  }
  if (args->off2 < comp) {
    lseek(fd_src, args->off2, SEEK_END);
  }
  close(fd_src);
  close(fd_dest);
  free(buf);
  free(args);
  return EXIT_SUCCESS;
}

void init_args(arg *s) {
  s->flags = DEFAULT_FLAGS;
  s->off1 = 0;
  s->off2 = 0;
  s->fichier_src = NULL;
  s->fichier_dest = NULL;
}
