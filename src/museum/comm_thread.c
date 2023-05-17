#include "main.h"
#include "comm_thread.h"


void *startCommThread(void *ptr) {
    MPI_Status status;

    packet_t packet;

    int clock = 0;

    debug("Comm thread Museum start");

    while (state != inFinish) {
        debug("Waiting for recv");
        MPI_Recv(&packet, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        clock = packet.ts > clock ? packet.ts + 1 : clock + 1;

        switch (status.MPI_TAG) {
            case RESERVE:
                debug("job reserved");
                sem_post(&jobReserveMut);
                break;
            case NEW_JOB:
                debug("new job... doing nothing");
            default:
                break;
        }

    }
}