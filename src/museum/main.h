#ifndef MAIN_H
#define MAIN_H


#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>


#include "../common.h"


extern int rank;
extern int size;
extern int lamport_time;

extern sem_t jobReserveMut;

extern pthread_t threadComm;

#endif