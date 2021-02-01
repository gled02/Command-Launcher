#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

typedef struct pwd pwd;

struct pwd {
  DIR *dir;
  struct dirent *file;
  char name[256];
  char path[256];
  pwd *next;
};

void set_pwd(pwd *source, DIR *dir, pwd *next);
struct dirent *get_file(DIR *dir);
pwd *check_pwd(pwd *s);

int main(void) {
  DIR *dir = opendir(".");
  if (dir == NULL) {
    perror("dir");
    exit(EXIT_FAILURE);
  }
  pwd *d = malloc(sizeof(pwd));
  set_pwd(d, dir, NULL);
  d = check_pwd(d);
  while (d->next != NULL) {
    printf("%s/", d->name);
    d = d->next;
  }
  printf("%s\n", d->name);
  return EXIT_SUCCESS;
}

void set_pwd(pwd *source, DIR *dir, pwd *next) {
  source->dir = dir;
  source->file = get_file(dir);
  if (next == NULL) {
    strcpy(source->path, ".");
  } else {
    strcpy(source->path, "..");
    strcat(source->path, "/");
    strcat(source->path, next->path);
  }
  source->name[0] = '\0';
  source->next = next;
}

struct dirent *get_file(DIR *dir) {
  struct dirent *file = NULL;
  rewinddir(dir);
  while((file = readdir(dir))) {
    if (strcmp(file->d_name, ".") == 0) {
      return file;
    }
  }
  return NULL;
}

pwd *check_pwd(pwd *s) {
  struct dirent *file = NULL;
  ino_t current = 1;
  ino_t previous = 1;
  rewinddir(s->dir);
  while ((file = readdir(s->dir))) {
    if (strcmp(file->d_name, ".") == 0) {
      current = file->d_ino;
    }
    if (strcmp(file->d_name, "..") == 0) {
      previous = file->d_ino;
    }
  }
  rewinddir(s->dir);
  if (current == previous) {
    return s;
  }
  while ((file = readdir(s->dir))) {
    if (strcmp(file->d_name, "..") == 0) {
      char relatif_path[256];
      strcpy(relatif_path, "..");
      strcat(relatif_path, "/");
      strcat(relatif_path, s->path);
      DIR *dir = opendir(relatif_path);
      if (dir == NULL) {
        perror("dir");
        exit(EXIT_FAILURE);
      }
      while ((file = readdir(dir))) {
        if (file->d_ino == s->file->d_ino) {
          strcpy(s->name, file->d_name);
          pwd *prev = malloc(sizeof(pwd));
          set_pwd(prev, dir, s);
          return check_pwd(prev);
        }
      }
    }
  }
  perror("Path error");
  exit(EXIT_FAILURE);
}
