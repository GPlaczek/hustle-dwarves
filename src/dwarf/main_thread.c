#include "main.h"
#include "main_thread.h"


void mainLoop() {
    srandom(rank);

    int jobToReq = -1;

    debug("Main thread start");

    while (state != inFinish) {
        switch (state) {
            case waitForNewJob:
                {
                    println("wait for new job");

                    pthread_mutex_lock(&queueJobsMut);
                    pthread_cond_wait(&newJobReceived, &queueJobsMut);

                    // packet_t *job = (packet_t *) jobs.data[jobs.rear];
                    // jobToReq = jobs.rear;

                    // debug("got new job: data: %d src: %d ts: %d", job->data, job->src, job->ts);

                    // pthread_cond_signal(&newJobProcessed);
                    pthread_mutex_unlock(&queueJobsMut);

                    changeState(newJobArrived);

                    break;
                }
            case newJobArrived:
                {
                    pthread_mutex_lock(&queueJobsMut);
                    packet_t *job = (packet_t *) jobs.data[jobToReq];

                    packet_t *pkt = malloc(sizeof(packet_t));

                    // museum id bits 0-7
                    // job id bits 8-19
                    // request timestamp bits 20-31
                    pkt->data = 0;
                    pkt->data |= (job->src & 0xFF);
                    pkt->data |= ((job->data & 0xFFF) << 8);
                    pkt->data |= ((lamport_time & 0xFFF) << 20);
                    // job arrived bits 0-15
                    // job requested bits 16-31
                    job->ts = (job->ts & 0xFFFF) | ((lamport_time & 0xFFFF) << 16);
                    // println("request for new job %d: museum: %d, job ts: %d, data: %d", jobToReq, job->src, job->ts, pkt->data);
                    pthread_mutex_unlock(&queueJobsMut);


                    for (int i = 0; i < size; i++) {
                        if (i != rank) {
                            sendPacket(pkt, i, REQ_JOB);
                        }
                    }

                    pthread_cond_signal(&newJobProcessed);

                    free(pkt);

                    changeState(waitForNewJob);
                    break;
                }


                
            default:
                break;
        }
    }
}
