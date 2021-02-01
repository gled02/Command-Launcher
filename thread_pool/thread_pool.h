//  Partie interface du module thread_pool

#ifndef THREADPOOL__H
#define THREADPOOL__H

//  struct threads threads : structure capable de gérer un tableu de threads.
//  Chaque champs répresente une information associée au thread.
typedef struct threads threads;

//  ini_threads: crée un objet de type threads. Les paramètres indiquent
//  respectivement le nombre de threads a initialisé, le nombre de connexion
//  maximal pour chaque thread, le nombre maximal des threads et la taille de
//  la shm. Renvoie NULL en cas de dépassement de capacité ou d'erreur,
//  un pointeur vers l'objet en cas de succès.
extern threads *ini_threads(size_t min_thread, size_t max_con,
    size_t max_thread, size_t shm_size);

//  connected_thread_to: distribue un thread a utilisé pour un nouveau client.
//   Renvoie le numéro de thread en cas de succès et -1 en cas d'échec.
extern int connected_thread_to(threads *th);

//  used_thread: décrémente le nombre des threads disponibles après la
//  déconnexion d'un client. Renvoie 1 si le thread est réinitialisé, revoie 2
//  s'il y a un changement d'état, 0 en cas de succès et -1 en cas d'échec.
extern int used_thread(threads *th, size_t number);

//  thread_dispose: si chaque élément de th ne vaut pas NULL, libère les
//  ressources allouées à th puis affecte la valeur NULL à th. (libère la
//  structure threads).
extern void thread_dispose(threads *th);

#endif
