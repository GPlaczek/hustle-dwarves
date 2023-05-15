#ifndef MAIN_H
#define MAIN_H


#include <stddef.h>
#include <unistd.h>
#include <string.h>


#include "../common.h"


extern int rank;
extern int size;
extern int lamport_time;

extern pthread_t threadComm;

#endif
