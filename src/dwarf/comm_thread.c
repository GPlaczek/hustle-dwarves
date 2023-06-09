#include "main.h"
#include "comm_thread.h"


void *startCommThread(void *ptr) {
    MPI_Status status;

    packet_t packet = {0};

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
                jobData *job = malloc(sizeof(jobData));
                job->museum_id = packet.museum_id;
                job->id = packet.id;
                job->ack_count = packet.ack_count;

                pthread_mutex_lock(&queueJobsMut);
                addNode(&jobs, job);
                pthread_mutex_unlock(&queueJobsMut);
                sem_post(&waitNewJobSem);

                if (state == waitForJobAccess) {
                    packet_t pkt;
                    pkt.museum_id = job->museum_id;
                    pkt.id = job->id;
                    pkt.request_ts = req_lamport;

                    for (int i = 0; i < size; i++) {
                        if (i != rank) {
                            sendPacket(&pkt, i, REQ_JOB);
                        }
                    }
                }

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

                // debug("enqueue %d %d %d", req->dwarf_id, req->job->museum_id, req->job->id);
                
                pthread_mutex_lock(&jobsRequestsMut);
                addNode(&jobs_requests, req);
                pthread_mutex_unlock(&jobsRequestsMut);

                int job_exists = 0;

                packet_t pkt;
                pkt.ack_count = packet.ack_count;
                pkt.id = packet.id;
                pkt.museum_id = packet.museum_id;
                pkt.request_ts = packet.request_ts;

                pthread_mutex_lock(&queueJobsMut);

                if (jobs.head == NULL) {
                    // debug("SEND1 ACK to %d", packet.src);
                    sendPacket(&pkt, packet.src, ACK_JOB);
                    pthread_mutex_unlock(&queueJobsMut);
                    break;
                }

                Node *current = jobs.head;
                while (current != NULL) {
                    jobData *job = (jobData *) current->data;

                    // debug("%d %d | %d %d | %d %d", job->id, packet.id, job->museum_id, packet.museum_id, req_lamport, packet.request_ts);

                    if (job->id == packet.id && 
                        job->museum_id == packet.museum_id) {
                        job_exists = 1;
                        if (req_lamport > packet.request_ts ||
                            state == jobAccessed || 
                            state == waitForPortal ||
                            state == inWork ||
                            req_lamport == 0) {
                            // debug("SEND2 ACK to %d", packet.src);
                            sendPacket(&pkt, packet.src, ACK_JOB);
                            break;
                        }
                    }
                    current = current->next;
                }

                pthread_mutex_unlock(&queueJobsMut);

                if (job_exists == 0) {
                    // debug("SEND3 ACK to %d", packet.src);
                    sendPacket(&pkt, packet.src, ACK_JOB);
                }

                break;
            }
            case ACK_JOB:
            {
                debug("ack job arrived from: %d museum: %d id: %d rts: %d", packet.src, packet.museum_id, packet.id, packet.request_ts);

                int on_duty = 0;

                pthread_mutex_lock(&queueJobsMut);
                // increase ack count for this job
                Node *current = jobs.head;

                while (current != NULL) {
                    jobData *job = (jobData *) current->data;
                    if (job->museum_id == packet.museum_id &&
                        job->id == packet.id) {
                        job->ack_count++;
                        // debug("ack for job %d %d: %d", job->museum_id, job->id, job->ack_count);

                        // on duty
                        if (job->ack_count == DWARVES - 1) {
                            debug("ON DUTY!");
                            on_duty = 1;
                            break;
                        }
                    }
                    current = current->next;
                }

                pthread_mutex_lock(&jobsRequestsMut);

                // distribute jobs
                if (on_duty) {
                    Node *current_job = jobs.head;
                    jobData *job = (jobData *) current_job->data;

                    Node *current_request;
                    request *req;

                    // Take the first job for myself
                    packet_t pkt;
                    pkt.museum_id = job->museum_id;
                    pkt.id = job->id;
                    pkt.request_ts = job->request_ts;
                    pkt.ack_count = job->ack_count;

                    // send reserve right away
                    for (int k = 0; k < size; k++) {
                        if (k != rank) {
                            sendPacket(&pkt, k, RESERVE);
                        }
                    }
                    removeNode(&jobs, job);

                    while (current_request != NULL) {
                        current_request = jobs_requests.head;
                        req = current_request->data;
                        int museumToDequeue = -1;
                        int jobToDequeue = -1;

                        current_job = jobs.head;
                        job = current_job->data;
                        while (current_job != NULL) {
                            // debug("REQ %d %d %d | %d %d", req->dwarf_id, req->job->museum_id, job->museum_id, req->job->id, job->id);
                            // In theory, it should work but I haven't seen it do anything actually
                            if (req->job->museum_id == job->museum_id &&
                                req->job->id == job->id) {
                                // send reserve to museum & dwarves
                                // if dwarf is in waitForJobAccess state
                                debug("Send TAKE to %d", req->dwarf_id);
                                sendPacket(&pkt, req->dwarf_id, TAKE);
                                museumToDequeue = req->job->museum_id;
                                jobToDequeue = req->job->id;
                                break;
                            }
                        }

                        // It was here before, I think that this loop can be integrated
                        // with the above one but I left it here
                        current_request = jobs_requests.head;
                        while (current_request != NULL) {
                            Node *next_request = current_request->next;
                            request *req = (request *) current_request->data;
                            if (req != NULL && req->job != NULL) {
                                if (museumToDequeue == req->job->museum_id &&
                                    jobToDequeue == req->job->id) {
                                    // debug("dequeue %d %d %d", req->dwarf_id, req->job->museum_id, req->job->id);
                                    removeNode(&jobs_requests, req);
                                }
                            }
                            current_request = next_request;
                        }
                        if (job != NULL) removeNode(&jobs, job);
                        removeNode(&jobs_requests, current_request);
                    }
                    sem_post(&jobAccessGranted);
                }
                pthread_mutex_unlock(&jobsRequestsMut);
                pthread_mutex_unlock(&queueJobsMut);
                break;
            }
            case TAKE:
            {   
                debug("take arrived from: %d museum: %d id: %d", packet.src, packet.museum_id, packet.id);

                pthread_mutex_lock(&queueJobsMut);

                if (jobs.head == NULL) {
                    break;
                }

                Node *current_job = jobs.head;
                while (current_job != NULL) {
                    Node *next_job = current_job->next;
                    jobData *job = (jobData *) current_job->data;

                    if (job->museum_id == packet.museum_id &&
                        job->id == packet.id) {
                        debug("Job taken: %d %d", job->museum_id, job->id);
                        removeNode(&jobs, job);
                        break;
                    }
                    current_job = next_job;
                }

                pthread_mutex_unlock(&queueJobsMut);

                packet_t pkt;
                pkt.museum_id = packet.museum_id;
                pkt.id = packet.id;
                pkt.request_ts = packet.request_ts;
                pkt.ack_count = packet.ack_count;

                for (int i = 0; i < size; i++) {
                    if (i != rank) {
                        sendPacket(&pkt, i, RESERVE);
                    }
                }

                sem_post(&jobAccessGranted);
                break;
            }
            case RESERVE:
            {
                debug("reserve arrived from: %d museum: %d id: %d", packet.src, packet.museum_id, packet.id);

                pthread_mutex_lock(&queueJobsMut);
                
                // remove reserved job from jobs
                Node *current_job = jobs.head;
                while (current_job != NULL) {
                    Node *next_job = current_job->next;
                    jobData *job = (jobData *) current_job->data;

                    if (job->museum_id == packet.museum_id &&
                        job->id == packet.id) {
                        debug("Job reserved: %d %d", job->museum_id, job->id);
                        removeNode(&jobs, job);
                        break;
                    }
                    current_job = next_job;
                }

                pthread_mutex_unlock(&queueJobsMut);

                // remove reserved job from requests
                pthread_mutex_lock(&jobsRequestsMut);
                Node *current_request = jobs_requests.head;
                while (current_request != NULL) {
                    Node *next_request = current_request->next;
                    request *req = (request *) current_request->data;
                    if (req != NULL && req->job != NULL) {
                        if (packet.museum_id == req->job->museum_id &&
                            packet.id == req->job->id) {
                            // debug("dequeue %d %d %d", req->dwarf_id, req->job->museum_id, req->job->id);
                            removeNode(&jobs_requests, req);
                        }
                    }
                    current_request = next_request;
                }
                pthread_mutex_unlock(&jobsRequestsMut);

                break;
            }
            case REQ_PORTAL:
            {
                debug("portal request arrived from: %d portal id: %d rts: %d", packet.src, packet.id, packet.request_ts);
                
                packet_t pkt;
                pkt.id = packet.id;
                pkt.museum_id = packet.museum_id;
                pkt.ack_count = packet.ack_count;
                pkt.request_ts = packet.request_ts;

                // dwarf is not working - send ack
                if (state != inWork && state != waitForPortal) {
                    sendPacket(&pkt, packet.src, ACK_PORTAL);
                    break;
                }

                // add request to the requests list
                portalData *request = malloc(sizeof(portalData));
                request->ack_count = 0;
                request->dwarf_id = packet.src;
                request->id = 0;
                request->request_ts = packet.request_ts;

                pthread_mutex_lock(&portalsRequestsMut);
                addNode(&portals_requests, request);
                pthread_mutex_unlock(&portalsRequestsMut);

                break;
            }
            case ACK_PORTAL:
            {   
                debug("ack portal arrived from: %d, portal: %d", packet.src, packet.id);

                // increase portal ack counter
                pthread_mutex_lock(&portalsAckMut);
                portal_ack++;
                pthread_mutex_unlock(&portalsAckMut);

                // portal accessed
                if (portal_ack >= DWARVES - PORTALS) {
                    sem_post(&waitForPortalAccess);
                }

                break;
            }
            default:
                break;
        }
    }

    return NULL;
}
