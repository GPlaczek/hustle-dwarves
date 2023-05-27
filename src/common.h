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
#define debug(FORMAT,...) printf("%c[%d;%dm [ts: %d] [" TAG " %d] " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, lamport_time, rank, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

// makro println - to samo co debug, ale wyświetla się zawsze
#define println(FORMAT,...) printf("%c[%d;%dm [ts: %d] [" TAG " %d] " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, lamport_time, rank, ##__VA_ARGS__, 27,0,37);

typedef struct {
    int museum_id;  // museum id
    int id;         // id of job in museum
    int request_ts; // timestamp of job request
    int ack_count;  // number of acks for job
} jobData;

typedef struct {
    int id;          // portal id
    int request_ts;   // timestamp of portal request
    int ack_count;   // number of acks for portal
} portalData;

typedef struct {
    int ts;         // timestamp - Lamport clock
    int src;        // packet source
    
    int museum_id;
    int id;
    int request_ts;
    int ack_count;
} packet_t;


#define NITEMS              6

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
    waitForNewJob,
    newJobArrived,
    waitForJobAccess,
    jobAccessed,
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
