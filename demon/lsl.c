#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define NB_MODE 10

mode_t flags[NB_MODE] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP,
  S_IROTH, S_IWOTH, S_IXOTH};

char flags_char[NB_MODE] = {'r', 'w', 'x', 'r', 'w', 'x', 'r', 'w', 'x'};

typedef struct file file;

struct file {
  char* name;
  char path[1024];
};

void *run(void* arg);
int display(int argc, char** argv);
int stat_file(struct stat *statbuf, char* filename);
char *permissions(struct stat *statbuf);

int main(int argc, char** argv) {
  if (argc < 2)  {
    perror("Incorrect number of arguments\n");
    exit(EXIT_FAILURE);
  }
  if (display(argc, argv) == -1) {
    exit(EXIT_FAILURE);
  }
  return EXIT_SUCCESS;
}

void *run(void* arg) {
  file *f = (file*)arg;
  struct stat *statbuf = malloc(sizeof(struct stat));
  if (stat(f->path, statbuf) == -1) {
    perror("stat");
    return NULL;
  }
  if (stat_file(statbuf, f->name) == -1) {
    perror("run : file not found\n");
    exit(EXIT_FAILURE);
  }
  free(f);
  return NULL;
}

int display(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    DIR *rep = opendir(argv[i]);
    if (rep == NULL) {
      pthread_t thread;
      file *s = malloc(sizeof(file));
      s->name = argv[i];
      strcpy(s->path, argv[i]);
      if (pthread_create(&thread, NULL, run, s) != 0) {
        perror("display : error pthread_create\n");
        exit(EXIT_FAILURE);
      }
    } else {
      struct dirent *fichier = NULL;
      while ((fichier = readdir(rep))) {
        pthread_t thread;
        file *s = malloc(sizeof(file));
        s->name = fichier->d_name;
        strcpy(s->path, argv[i]);
        if (argv[i][strlen(argv[i]) - 1] != '/') {
          strcat(s->path, "/");
        }
        strcat(s->path, fichier->d_name);
        if (pthread_create(&thread, NULL, run, s) != 0) {
          perror("display : error pthread_create\n");
          exit(EXIT_FAILURE);
        }
      }
    }
  }
  pthread_exit(NULL);
  return 0;
}

int stat_file(struct stat *statbuf, char* filename) {
  struct passwd *user = getpwuid(statbuf->st_uid);
  struct group *group = getgrgid(statbuf->st_gid);
  char* perm = permissions(statbuf);
  char* time = ctime(&statbuf->st_mtime);
  time[strlen(time) - 1] = '\0';
  printf("%ld\t%s\t%ld\t%d\t%s\t%ld\t%s\t%s\n", statbuf->st_ino, perm,
  statbuf->st_nlink, user->pw_uid, group->gr_name, statbuf->st_size,
  time, filename);
  free(perm);
  return 0;
}

char *permissions(struct stat *statbuf) {
  char* perm = malloc(sizeof(char) * NB_MODE);
  for (int i = 1; i < NB_MODE; ++i) {
    if (statbuf->st_mode & flags[i - 1]) {
      perm[i] = flags_char[i - 1];
    } else {
      perm[i] = '-';
    }
  }
  if (S_ISBLK(statbuf->st_mode)) {
    perm[0] = 'b';
  } else if (S_ISCHR(statbuf->st_mode)) {
    perm[0] = 'c';
  } else if (S_ISDIR(statbuf->st_mode)) {
    perm[0] = 'd';
  } else if (S_ISFIFO(statbuf->st_mode)) {
    perm[0] = 'p';
  } else if (S_ISLNK(statbuf->st_mode)) {
    perm[0] = 'l';
  } else if (S_ISSOCK(statbuf->st_mode)) {
    perm[0] = 's';
  } else {
    perm[0] = '-';
  }
  if (statbuf->st_mode & S_ISUID) {
    perm[3] = 's';
  }
  if (statbuf->st_mode & S_ISGID) {
    perm[6] = 's';
  }
  if (statbuf->st_mode & S_ISVTX) {
    perm[9] = 's';
  }
  return perm;
}
