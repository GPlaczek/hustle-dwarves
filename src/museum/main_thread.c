#include "main.h"
#include "main_thread.h"
#include "../queue.h"

void mainLoop() {
    srandom(rank);
    
    Queue jobs;
    initQueue(&jobs);

    debug("Main thread Dwarf start");

    while (state != inFinish) {
        switch (state) {
            case generateJob:
                int new_job = lamport_time;
                debug("Generated new job: %d", new_job);
                enqueue(&jobs, new_job);
                changeState(sendNewJob);
                debug("New State: %d\n", state);
                sleep(rand() % 5 + 1);
                break;
            case sendNewJob:
                packet_t *pkt = malloc(sizeof(packet_t));
                pkt->ts = lamport_time;
                pkt->src = rank;
                pkt->data = jobs.data[jobs.front];

                for (int i = 0; i < size; i++) {
                    if (i != rank) {
                        sendPacket(pkt, i, NEW_JOB);
                    }
                }

                changeState(waitForReserve);
                break;
            case waitForReserve:
                sem_wait(&jobReserveMut);
                debug("Job taken: %d", jobs.data[jobs.front]);
                changeState(generateJob);
                dequeue(&jobs);
                break;
            default:
                break;
        }
    }

    debug("main thread finish\n");
}
