#include "main.h"
#include "comm_thread.h"


void *startCommThread(void *ptr) {
    MPI_Status status;

    packet_t packet;

    debug("Comm thread Museum start");

    while (state != inFinish) {
        debug("Waiting for recv");
        MPI_Recv(&packet, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        pthread_mutex_lock(&lamportMut);
        lamport_time = packet.ts > lamport_time ? packet.ts + 1 : lamport_time + 1;
        pthread_mutex_unlock(&lamportMut);

        switch (status.MPI_TAG) {
            case RESERVE:
            {
                debug("job reserved dwarf: %d, museum_id: %d, job id: %d", packet.src, packet.museum_id, packet.id);
                
                if (packet.museum_id == rank) {
                    sem_post(&jobReserveMut);
                }
                break;
            }
            default:
                break;
        }

    }

    return NULL;
}
