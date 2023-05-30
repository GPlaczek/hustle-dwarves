#include "main.h"
#include "comm_thread.h"


typedef struct {
    int dwarf_id;
    jobData *job;
} request;


void *startCommThread(void *ptr) {
    MPI_Status status;

    packet_t packet = {0};

    Queue requests;

    initQueue(&requests);

    debug("Comm thread Dwarf start");

    while (state != inFinish) {
        debug("Waiting for recv");
        MPI_Recv(&packet, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        pthread_mutex_lock(&lamportMut);
        lamport_time = packet.ts > lamport_time ? packet.ts + 1 : lamport_time + 1;
        pthread_mutex_unlock(&lamportMut);


        switch (status.MPI_TAG) {
            case NEW_JOB:
            {
                debug("new job src: %d ts: %d museum: %d id: %d", packet.src, packet.ts, packet.museum_id, packet.id);
                
                // add job to queue
                pthread_mutex_lock(&queueJobsMut);
                jobData *job = malloc(sizeof(jobData));
                job->museum_id = packet.museum_id;
                job->id = packet.id;
                job->request_ts = lamport_time;
                job->ack_count = packet.ack_count;

                enqueue(&jobs, job);
                pthread_mutex_unlock(&queueJobsMut);
                
                // send job requests & add request to requests queue
                packet_t *pkt = malloc(sizeof(packet_t));
                pkt->museum_id = job->museum_id;
                pkt->id = job->id;
                pkt->request_ts = job->request_ts;
                pkt->ack_count = 0;

                for (int i = 0; i < size; i++) {
                    if (i != rank) {
                        sendPacket(pkt, i, REQ_JOB);
                    }
                }
                free(pkt);

                request *req = malloc(sizeof(request));
                req->dwarf_id = rank;
                req->job = (jobData *) jobs.data[jobs.rear];
                enqueue(&requests, req);
                
                sem_post(&waitNewJobSem);
                break;
            }
            case REQ_JOB:
            {
                debug("req job arrived from %d: museum: %d request_ts: %d id: %d", packet.src, packet.museum_id, packet.request_ts, packet.id);

                // add request to requests queue
                request *req = malloc(sizeof(request));
                req->dwarf_id = packet.src;

                jobData *_job = malloc(sizeof(jobData));
                _job->museum_id = packet.museum_id;
                _job->id = packet.id;
                _job->ack_count = packet.ack_count;
                _job->request_ts = packet.request_ts;

                req->job = _job;

                debug("enqueue %d %d %d", req->dwarf_id, req->job->museum_id, req->job->id);
                enqueue(&requests, req);

                int job_exists = 0;

                packet_t *pkt = malloc(sizeof(packet_t));
                pkt->ack_count = packet.ack_count;
                pkt->id = packet.id;
                pkt->museum_id = packet.museum_id;
                pkt->request_ts = packet.request_ts;

                pthread_mutex_lock(&queueJobsMut);

                if (jobs.front < 0 || jobs.rear < 0) {
                    sendPacket(pkt, packet.src, ACK_JOB);
                    pthread_mutex_unlock(&queueJobsMut);
                    free(pkt);
                    break;
                }

                for (int i = jobs.front; i <= jobs.rear; i++) {
                    jobData *job = (jobData *) jobs.data[i];
                    
                    debug("%d %d | %d %d | %d %d", job->id, packet.id, job->museum_id, packet.museum_id, job->request_ts, packet.request_ts);

                    if (job->id == packet.id && 
                        job->museum_id == packet.museum_id) {
                        job_exists = 1;
                        if (job->request_ts < packet.request_ts) {
                            sendPacket(pkt, packet.src, ACK_JOB);
                            break;
                        }
                    }
                }

                pthread_mutex_unlock(&queueJobsMut);

                if (job_exists == 0) {
                    sendPacket(pkt, packet.src, ACK_JOB);
                }

                free(pkt);
                break;
            }
            case ACK_JOB:
            {
                debug("ack job arrived from: %d museum: %d id: %d rts: %d", packet.src, packet.museum_id, packet.id, packet.request_ts);

                int on_duty = 0;

                pthread_mutex_lock(&queueJobsMut);

                if (jobs.front == -1 || jobs.rear == -1) {
                    break;
                }

                // increase ack count for this job
                for (int i = jobs.front; i <= jobs.rear; i++) {
                    jobData *local_job = (jobData *) jobs.data[i];
                    if (local_job->museum_id == packet.museum_id &&
                        local_job->id == packet.id) {
                        local_job->ack_count++;
                        debug("ack for job %d %d: %d", local_job->museum_id, local_job->id, local_job->ack_count);

                        // on duty
                        if (local_job->ack_count == DWARVES - 1) {
                            debug("ON DUTY!");
                            on_duty = 1;
                            break;
                        }
                    }
                }

                for (int i = requests.front; i <= requests.rear; i++) {
                    request *req = (request *) requests.data[i];
                    debug("REQ %d %d %d", req->dwarf_id, req->job->museum_id, req->job->id);
                }

                // distribute jobs
                if (on_duty && requests.front > -1 && requests.rear > -1) {
                    for (int j = jobs.front; j <= jobs.rear; j++) {
                        jobData *job = (jobData *) jobs.data[j];

                        packet_t *pkt = malloc(sizeof(packet_t));
                        pkt->museum_id = job->museum_id;
                        pkt->id = job->id;
                        pkt->request_ts = job->request_ts;
                        pkt->ack_count = job->ack_count;

                        int museumToDequeue = -1;
                        int jobToDequeue = -1;

                        for (int i = requests.front; i <= requests.rear; i++) {
                            request *req = (request *) requests.data[i];

                            debug("REQ %d %d %d | %d %d", req->dwarf_id, req->job->museum_id, job->museum_id, req->job->id, job->id);
                            
                            if (req->job->museum_id == job->museum_id &&
                                req->job->id == job->id) {
                                
                                // send reserve to museum & dwarves
                                // if dwarf is in waitForJobAccess state
                                if (req->dwarf_id == rank &&
                                    state == waitForJobAccess) {
                                    for (int k = 0; k < size; k++) {
                                        if (k != rank) {
                                            sendPacket(pkt, k, RESERVE);
                                        }
                                    }
                                    sem_post(&jobAccessGranted);
                                } else if (req->dwarf_id != rank) {
                                    debug("Send TAKE to %d", req->dwarf_id);
                                    // sendPacket(pkt, req->dwarf_id, TAKE);
                                }
                                museumToDequeue = req->job->museum_id;
                                jobToDequeue = req->job->id;
                                break;
                            }
                        }

                        for (int i = requests.front; i <= requests.rear; i++) {
                            request *req = (request *) requests.data[i];
                            if (museumToDequeue == req->job->museum_id &&
                                jobToDequeue == req->job->id) {
                                debug("dequeue %d %d %d", req->dwarf_id, req->job->museum_id, req->job->id);
                                dequeueAt(&requests, i);
                                free(req);
                            }
                        }
                        free(pkt);
                        
                        dequeueAt(&jobs, j);
                        free(job);
                    }
                }

                pthread_mutex_unlock(&queueJobsMut);
                break;
            }
            case TAKE:
            {   
                debug("take arrived from: %d museum: %d id: %d", packet.src, packet.museum_id, packet.id);

                pthread_mutex_lock(&queueJobsMut);

                if (jobs.front < 0 || jobs.rear < 0) {
                    break;
                }

                for (int i = jobs.front; i <= jobs.rear; i++) {
                    jobData *job = (jobData *) jobs.data[i];

                    if (job->museum_id == packet.museum_id &&
                        job->id == packet.id) {
                        debug("Job taken: %d %d", job->museum_id, job->id);
                        dequeueAt(&jobs, i);
                        break;
                    }
                }

                pthread_mutex_unlock(&queueJobsMut);

                packet_t *pkt = malloc(sizeof(packet_t));
                pkt->museum_id = packet.museum_id;
                pkt->id = packet.id;
                pkt->request_ts = packet.request_ts;
                pkt->ack_count = packet.ack_count;

                for (int i = 0; i < size; i++) {
                    if (i != rank) {
                        sendPacket(pkt, i, RESERVE);
                    }
                }

                free(pkt);

                sem_post(&jobAccessGranted);
                break;
            }
            case RESERVE:
            {
                debug("reserve arrived from: %d museum: %d id: %d", packet.src, packet.museum_id, packet.id);

                pthread_mutex_lock(&queueJobsMut);

                if (jobs.front < 0 || jobs.rear < 0) {
                    break;
                }

                for (int i = jobs.front; i <= jobs.rear; i++) {
                    jobData *job = (jobData *) jobs.data[i];

                    if (job->museum_id == packet.museum_id &&
                        job->id == packet.id) {
                        debug("Job reserved: %d %d", job->museum_id, job->id);
                        dequeueAt(&jobs, i);
                        break;
                    }
                }

                pthread_mutex_unlock(&queueJobsMut);

                break;
            }
            case REQ_PORTAL:
            {
                debug("portal request arrived from: %d portal id: %d rts: %d", packet.src, packet.id, packet.request_ts);
                
                packet_t *pkt = malloc(sizeof(packet_t));
                pkt->id = packet.id;
                pkt->museum_id = packet.museum_id;
                pkt->ack_count = packet.ack_count;
                pkt->request_ts = packet.request_ts;

                if (portals.front < 0 || portals.rear < 0) {
                    sendPacket(pkt, packet.src, ACK_PORTAL);
                    break;
                }

                int portal_exists = 0;

                pthread_mutex_lock(&queuePortalsMut);

                for (int i = portals.front; i <= portals.rear; i++) {
                    portalData *portal = (portalData *) portals.data[i];

                    debug("%d %d | %d %d", portal->id, packet.id, portal->request_ts, packet.request_ts);

                    if (portal->id == packet.id) {
                        portal_exists = 1;
                        if (portal->request_ts < packet.request_ts) {
                            sendPacket(pkt, packet.src, ACK_PORTAL);
                        }
                    }
                }

                pthread_mutex_unlock(&queuePortalsMut);
                
                if (portal_exists == 0) {
                    sendPacket(pkt, packet.src, ACK_PORTAL);
                }

                free(pkt);

                break;
            }
            case ACK_PORTAL:
            {   
                debug("ack portal arrived from: %d, portal: %d", packet.src, packet.id);

                int portal_accessed = -1;

                pthread_mutex_lock(&queuePortalsMut);
                
                // increase ack count for this portal
                for (int i = portals.front; i <= portals.rear; i++) {
                    portalData *portal = (portalData *) portals.data[i];

                    if (portal->id == packet.id) {
                        portal->ack_count++;
                    }

                    debug("%d %d %d", portal->dwarf_id, rank, portal->ack_count)

                    if (portal->dwarf_id == rank &&
                        portal->ack_count == DWARVES - 1) {
                        debug("PORTAL ACCESSED!");
                        portal_accessed = portal->id;
                        // sem_post(&waitForPortalAccess);
                        dequeueAt(&portals, i);
                        free(portal);
                        break;
                    }
                }

                // if (portal_accessed != -1) {
                //     for (int i = portals.front; i <= portals.rear; i++) {
                //         portalData *portal = (portalData *) portals.data[i];

                //         if (portal->id == portal_accessed) {
                //             packet_t *pkt = malloc(sizeof(packet_t));

                //             pkt->ack_count = 0;
                //             pkt->id = portal->id;
                //             pkt->museum_id = -1;
                //             pkt->request_ts = portal->request_ts;

                //             sendPacket(pkt, portal->dwarf_id, ACK_PORTAL);

                //             dequeueAt(&portals, i);
                //             free(portal);
                //             free(pkt);
                //         }
                //     }
                // }

                pthread_mutex_unlock(&queuePortalsMut);

                break;
            }
            default:
                break;
        }
    }

    return NULL;
}