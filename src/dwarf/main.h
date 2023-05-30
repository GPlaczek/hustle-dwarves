#ifndef MAIN_H
#define MAIN_H


#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>


#include "../common.h"
#include "../list.h"


extern int rank;
extern int size;
extern int lamport_time;

extern pthread_t threadComm;
extern pthread_mutex_t queueJobsMut;
extern pthread_mutex_t queuePortalsMut;


extern sem_t waitNewJobSem;
extern sem_t waitForJobProcessed;
extern sem_t jobAccessGranted;
extern sem_t waitForPortalAccess;
extern List jobs;
extern List portals;



#endif
