#include "main.h"
#include "main_thread.h"


void mainLoop() {
    srandom(rank);

    int jobToReq = -1;

    debug("Main thread start");

    while (state != inFinish) {
        debug("MAIN STATE: %d", state);

        switch (state) {
            case waitForNewJob:
            {
                println("wait for new job");

                sem_wait(&waitNewJobSem);
                pthread_mutex_lock(&queueJobsMut);

                jobData *job = (jobData *) jobs.data[jobs.rear];
                jobToReq = jobs.rear;
                debug("got new job: museum: %d id: %d, request ts: %d", job->museum_id, job->id, job->request_ts);

                pthread_mutex_unlock(&queueJobsMut);

                changeState(newJobArrived);
                break;
            }
            case newJobArrived:
            {
                debug("New Job Arrived");
                pthread_mutex_lock(&queueJobsMut);
                jobData *job = (jobData *) jobs.data[jobToReq];
                job->request_ts = lamport_time;


                packet_t *pkt = malloc(sizeof(packet_t));                
                pkt->museum_id = job->museum_id;
                pkt->id = job->id;
                pkt->request_ts = job->request_ts;
                pkt->ack_count = 0;
                pthread_mutex_unlock(&queueJobsMut);
                sem_post(&waitForJobProcessed);

                println("request for new job museum: %d, request ts: %d, id: %d", pkt->museum_id, pkt->request_ts, pkt->id);
                for (int i = 0; i < size; i++) {
                    sendPacket(pkt, i, REQ_JOB);
                }

                changeState(waitForJobAccess);
                free(pkt);
                break;
            }
            case waitForJobAccess:
            {
                debug("wait for job access");

                sem_wait(&jobAccessGranted);

                changeState(jobAccessed);
                break;
            }

            case jobAccessed:
            {
                debug("Job Accessed!");
                changeState(inFinish);
                break;
            }
            default:
                break;
        }
    }
}
