#ifndef MAIN_H
#define MAIN_H


#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>


#include "../common.h"
#include "../queue.h"


extern int rank;
extern int size;
extern int lamport_time;

extern pthread_t threadComm;
extern pthread_mutex_t queueJobsMut;

extern pthread_cond_t newJobReceived;
extern pthread_cond_t newJobProcessed;

extern sem_t waitNewJobSem;
extern Queue jobs;



#endif
