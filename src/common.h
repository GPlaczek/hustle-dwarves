#ifndef COMMON_H
#define COMMON_H


#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>


#define TRUE        1
#define FALSE       0


extern int rank;
extern int size;
extern int lamport_time;




#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [ts: %d] [%d] " TAG ": " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, lamport_time, rank, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

// makro println - to samo co debug, ale wyświetla się zawsze
#define println(FORMAT,...) printf("%c[%d;%dm [ts: %d] [%d] " TAG ": " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, lamport_time, rank, ##__VA_ARGS__, 27,0,37);



typedef struct {
    int ts;         // timestamp - Lamport clock
    int src;
    int data;
} packet_t;

#define NITEMS              3

// signals
#define NEW_JOB             1
#define REQ_JOB             2
#define ACK_JOB             3
#define TAKE                4
#define RESERVE             5
#define REQ_PORTAL          6
#define ACK_PORTAL          7

// states
typedef enum {
    generateJob,
    sendNewJob,
    waitForReserve,
    newJobArrived,
    waitForJobAccess,
    onDuty,
    waitForPortal,
    inWork,
    inFinish
} state_t;




extern MPI_Datatype MPI_PACKET_T;
void init_packet_type();

void sendPacket(packet_t *pkt, int destination, int tag);


extern state_t state;
extern pthread_mutex_t stateMut;
extern pthread_mutex_t lamportMut;


void changeState(state_t newState);

#endif
