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

                packet_t *pkt = (packet_t *) packets.data[packets.rear];
                jobToReq = packets.rear;


                debug("got new job: src: %d ts: %d museum: %d id: %d", pkt->src, pkt->ts, pkt->museum_id, pkt->id);

                pthread_cond_signal(&newJobProcessed);
                pthread_mutex_unlock(&queueJobsMut);

                changeState(newJobArrived);

                break;
            }
            case newJobArrived:
            {
                pthread_mutex_lock(&queueJobsMut);
                packet_t *packet = (packet_t *) packets.data[jobToReq];

                packet_t *pkt = malloc(sizeof(packet_t));
                pkt = packet;

                pkt->request_ts = lamport_time;

                println("request for new job museum: %d, request ts: %d, id: %d", pkt->museum_id, pkt->request_ts, pkt->id);
                pthread_mutex_unlock(&queueJobsMut);


                for (int i = 0; i < size; i++) {
                    if (i != rank) {
                        sendPacket(pkt, i, REQ_JOB);
                    }
                }

                pthread_cond_signal(&newJobProcessed);

                changeState(waitForNewJob);
                break;
            }
            default:
                break;
        }
    }
}
