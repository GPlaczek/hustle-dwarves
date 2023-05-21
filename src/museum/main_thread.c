#include "main.h"
#include "main_thread.h"
#include "../queue.h"

void mainLoop() {
    srandom(rank);
    
    Queue jobs;
    initQueue(&jobs);

    debug("Main thread Museum start");

    while (state != inFinish) {
        switch (state) {
            case generateJob:
                int new_job = lamport_time;
                debug("Generated new job: %d", new_job);
                enqueue(&jobs, &new_job);
                changeState(sendNewJob);
                debug("New State: %d\n", state);
                sleep(rand() % 5 + 1);
                break;
            case sendNewJob: 
                {
                    packet_t *pkt = malloc(sizeof(packet_t));

                    int *job_id = (int *) jobs.data[jobs.front];

                    pkt->data = *job_id;

                    for (int i = 0; i < size; i++) {
                        if (i != rank) {
                            sendPacket(pkt, i, NEW_JOB);
                        }
                    }

                    free(pkt);

                    sem_wait(&jobReserveMut);
                    changeState(waitForReserve);
                    break;
                }
                
            case waitForReserve:
                int *job_id = (int *) jobs.data[jobs.front];

                debug("Wait for reserve: %d", *job_id);

                sem_wait(&jobReserveMut);
                changeState(generateJob);
                dequeue(&jobs);
                break;
            default:
                break;
        }
    }

    debug("main thread finish\n");
}
