#ifndef BWAPI_SEM_H
#define BWAPI_SEM_H

#ifndef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

static sem_t * arch_id;

static void lockArchive()
{
    const char* file = "archivelock43";
    arch_id=sem_open(file, O_CREAT, 0600, 1);

    if(arch_id == SEM_FAILED) {
        perror("parent sem_open");
        return;
    }
    //printf("waiting for process\n");
    if(sem_wait(arch_id) < 0) {
        //perror("sem_wait");
    }
}

static void releaseArchive()
{
    //arch_id=sem_open("mysem", O_CREAT, 0600, 0);
    if(arch_id == SEM_FAILED) {
        //perror("child sem_open");
        return;
    }
    //printf("Posting for parent\n");
    if(sem_post(arch_id) < 0) {
        //perror("sem_post");
    }
    if ( sem_close(arch_id) < 0 )
    {
        //perror("sem_close");
    }
}

#endif

/*
struct semaphore {
    pthread_mutex_t lock;
    pthread_cond_t nonzero;
    unsigned count;
};
typedef struct semaphore semaphore_t;

semaphore_t *semaphore_create(const char *semaphore_name);
semaphore_t *semaphore_open(const char *semaphore_name);
void semaphore_post(semaphore_t *semap);
void semaphore_wait(semaphore_t *semap);
void semaphore_close(semaphore_t *semap);
*/

#endif //BWAPI_SEM_H
