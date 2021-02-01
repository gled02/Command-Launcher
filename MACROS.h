//  Fichier .h contenant des macro-constantes communes au client, au threads et
// au démon

#ifndef MACROS_H
#define MACROS_H

#define SYNC "SYNC"
#define END  "END"
#define RST  "RST"
#define TUBE_DEMON   "../tubes_names/demon_tube"
#define TUBE_CLIENT  "../tubes_names/tube_client_"
#define SHM_NAME     "/shm_thread_"  //dans /dev/shm/

#define TUBE_NAME_LENGTH 32
#define PID_LENGTH       7
#define SHM_LENGTH       32

#define SHM_FLAG         0 // Le shm est initialisée
#define COMMANDE_FLAG    1 // Le client a entré la commande
#define RESULT_FLAG      2 // Le client peut récupérer le resultat

#endif
