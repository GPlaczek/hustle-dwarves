#include "main.h"
#include "main_thread.h"


void mainLoop() {
    srandom(rank);

    debug("Main thread start");

    while (state != inFinish) {
        debug("MAIN STATE: %d", state);

        switch (state) {
            case waitForNewJob:
            {
                println("wait for new job");

                sem_wait(&waitNewJobSem);
                pthread_mutex_lock(&queueJobsMut);
                
                if (jobs.head != NULL) {
                    jobData *job = (jobData *) jobs.tail->data;
                    debug("got new job: museum: %d id: %d, request ts: %d", job->museum_id, job->id, job->request_ts);
                }

                pthread_mutex_unlock(&queueJobsMut);

                changeState(waitForJobAccess);
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
                changeState(waitForPortal);
                break;
            }
            case waitForPortal:
            {
                debug("wait for portal");

                packet_t *pkt = malloc(sizeof(packet_t));
                pkt->ack_count = 0;
                pkt->id = rand() % PORTALS;
                pkt->museum_id = -1;
                pkt->request_ts = lamport_time;

                pthread_mutex_lock(&queuePortalsMut);

                portalData *portal_req = malloc(sizeof(portalData));
                portal_req->ack_count = 0;
                portal_req->id = pkt->id;
                portal_req->request_ts = pkt->request_ts;
                portal_req->dwarf_id = rank;

                addNode(&portals, portal_req);
                pthread_mutex_unlock(&queuePortalsMut);

                for (int i = 0; i < size; i++) {
                    if (i != rank) {
                        sendPacket(pkt, i, REQ_PORTAL);
                    }
                }

                free(pkt);

                sem_wait(&waitForPortalAccess);

                changeState(inWork);

                break;
            }
            case inWork:
            {
                debug("WORKING...");
                sleep(rand() % 5 + 1);
                changeState(waitForNewJob);
                break;
            }
            default:
                break;
        }
    }
}
