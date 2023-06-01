#include "common.h"


int rank;
int size;
int lamport_time = 0;


MPI_Datatype MPI_PACKET_T;

state_t state;

pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lamportMut = PTHREAD_MUTEX_INITIALIZER;


struct tagNames_t {
    const char *name;
    int tag;
} tagNames[] = {
    {"new job", NEW_JOB},
    {"job request", REQ_JOB},
    {"job acknowledge", ACK_JOB},
    {"job take", TAKE},
    {"job reserve", RESERVE},
    {"portal request", REQ_PORTAL},
    {"portal acknowledge", ACK_PORTAL}
};


const char *const tag2string(int tag) {
    for (int i = 0; i < sizeof(tagNames) / sizeof(struct tagNames_t); i++) {
        if (tagNames[i].tag == tag) {
            return tagNames[i].name;
        }
    }
    return "<unknown>";
}

void init_packet_type() {
    int blocklengths[NITEMS] = {1, 1, 1, 1, 1, 1};
    MPI_Datatype types[NITEMS] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offsets[NITEMS];
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, museum_id);
    offsets[3] = offsetof(packet_t, id);
    offsets[4] = offsetof(packet_t, request_ts);
    offsets[5] = offsetof(packet_t, ack_count);

    MPI_Type_create_struct(NITEMS, blocklengths, offsets, types, &MPI_PACKET_T);

    MPI_Type_commit(&MPI_PACKET_T);
}

void sendPacket(packet_t *pkt, int destination, int tag) {
    int freepkt = 0;
    if (pkt == 0) {
        pkt = malloc(sizeof(packet_t));
        freepkt = 1;
    }
    pkt->src = rank;

    pthread_mutex_lock(&lamportMut);
    lamport_time++;
    pthread_mutex_unlock(&lamportMut);

    pkt->ts = lamport_time;

    MPI_Send(pkt, 1, MPI_PACKET_T, destination, tag, MPI_COMM_WORLD);
    // debug("Sending %s from %d to %d, rts: %d", tag2string(tag), rank, destination, pkt->request_ts);

    if (freepkt) {
        free(pkt);
    }
}

void changeState(state_t newState) {
    pthread_mutex_lock(&stateMut);
    if (state == inFinish) {
        pthread_mutex_unlock(&stateMut);
        return;
    }
    state = newState;
    pthread_mutex_unlock(&stateMut);
}
