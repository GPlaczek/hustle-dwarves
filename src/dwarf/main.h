#ifndef MAIN_H
#define MAIN_H


#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>


#include "../common.h"
#include "../list.h"

typedef struct {
    int dwarf_id;
    jobData *job;
} request;


extern int rank;
extern int size;
extern int lamport_time;
extern int portal_ack;

extern pthread_t threadComm;
extern pthread_mutex_t queueJobsMut;
extern pthread_mutex_t portalsAckMut;
extern pthread_mutex_t jobsRequestsMut;
extern pthread_mutex_t portalsRequestsMut;


extern sem_t waitNewJobSem;

extern sem_t waitForJobProcessed;
extern sem_t jobAccessGranted;
extern sem_t waitForPortalAccess;

extern List jobs;
extern List jobs_requests;
extern List portals_requests;


#endif
