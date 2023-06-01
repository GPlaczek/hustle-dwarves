#include "main.h"
#include "main_thread.h"


void mainLoop() {
    srandom(rank);

    debug("Main thread start");

    while (state != inFinish) {
        // debug("MAIN STATE: %d", state);

        switch (state) {
            case waitForNewJob:
            {
                println("wait for new job");

                // if there were any portal requests during work - send ack to them
                pthread_mutex_lock(&portalsRequestsMut);
                Node *current_portal = portals_requests.head;
                while (current_portal != NULL) {
                    Node *next_request = current_portal->next;
                    portalData *portal = (portalData *) current_portal->data;

                    packet_t *pkt = malloc(sizeof(packet_t));
                    pkt->museum_id = -1;
                    pkt->id = portal->id;
                    pkt->request_ts = lamport_time;
                    pkt->ack_count = 0;

                    sendPacket(pkt, portal->dwarf_id, ACK_PORTAL);
                    free(pkt);
                    removeNode(&portals_requests, portal);

                    current_portal = next_request;
                }
                pthread_mutex_unlock(&portalsRequestsMut);

                int job_requested = 0;

                // if there are any queued jobs - ask for access
                pthread_mutex_lock(&queueJobsMut);
                Node *current_job = jobs.head;
                while (current_job != NULL) {
                    jobData *job = (jobData *) current_job->data;

                    packet_t *pkt = malloc(sizeof(packet_t));
                    pkt->museum_id = job->museum_id;
                    pkt->id = job->id;
                    pkt->request_ts = lamport_time;
                    pkt->ack_count = 0;

                    for (int i = 0; i < size; i++) {
                        if (i != rank) {
                            sendPacket(pkt, i, REQ_JOB);
                        }
                    }
                    free(pkt);
                    job_requested = 1;

                    request *req = malloc(sizeof(request));
                    req->dwarf_id = rank;
                    req->job = (jobData *) job;
                    req->job->request_ts = lamport_time;

                    pthread_mutex_lock(&jobsRequestsMut);
                    addNode(&jobs_requests, req);
                    pthread_mutex_unlock(&jobsRequestsMut);
                    
                    current_job = current_job->next;
                }
                pthread_mutex_unlock(&queueJobsMut);

                if (job_requested == 0) {
                    sem_wait(&waitNewJobSem);
                }

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

                pthread_mutex_lock(&portalsAckMut);
                portal_ack = 0;
                pthread_mutex_unlock(&portalsAckMut);

                // there is always a free portal
                if (DWARVES >= PORTALS) {
                    changeState(inWork);
                    break;
                }

                // send portal requests
                packet_t *pkt = malloc(sizeof(packet_t));
                pkt->ack_count = 0;
                pkt->id = 0;
                pkt->museum_id = -1;
                pkt->request_ts = lamport_time;
                
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
                pthread_mutex_lock(&portalsAckMut);
                portal_ack = 0;
                pthread_mutex_unlock(&portalsAckMut);

                debug("\t\t\t\t\t\t\t\t\tWORKING...");
                sleep(rand() % 20 + 10);
                changeState(waitForNewJob);
                debug("END WORKING");
                break;
            }
            default:
                break;
        }
    }
}
