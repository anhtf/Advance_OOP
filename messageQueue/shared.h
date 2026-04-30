#ifndef SHARED_H
#define SHARED_H

#define SHM_NAME "/my_shm"
#define SEM_EMPTY "/sem_empty"
#define SEM_FULL "/sem_full"

typedef struct {
    char data[256];
} shared_data_t;

#endif