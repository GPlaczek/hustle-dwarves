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
                
                pthread_mutex_lock(&queueJobsMut);
                jobData *job = malloc(sizeof(jobData));
                job->museum_id = packet.museum_id;
                job->id = packet.id;
                job->request_ts = 0;
                job->ack_count = packet.ack_count;

                enqueue(&jobs, job);
                pthread_mutex_unlock(&queueJobsMut);
                sem_post(&waitNewJobSem);
                changeState(waitForNewJob);
                // sem_wait(&waitForJobProcessed);
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

                debug("enqueue");
                enqueue(&requests, req);

                if (packet.src == rank) {
                    break;
                }

                int job_exists = 0;

                pthread_mutex_lock(&queueJobsMut);

                if (jobs.front < 0 || jobs.rear < 0) {
                    sendPacket(&packet, packet.src, ACK_JOB);
                    break;
                }

                for (int i = jobs.front; i <= jobs.rear; i++) {
                    jobData *job = (jobData *) jobs.data[i];
                    
                    debug("%d %d | %d %d | %d %d", job->id, packet.id, job->museum_id, packet.museum_id, job->request_ts, packet.request_ts);

                    if (job->id == packet.id && 
                        job->museum_id == packet.museum_id) {
                        job_exists = 1;
                        if (job->request_ts < packet.request_ts) {
                            sendPacket(&packet, packet.src, ACK_JOB);
                            break;
                        }
                    }
                }

                pthread_mutex_unlock(&queueJobsMut);

                if (job_exists == 0) {
                    sendPacket(&packet, packet.src, ACK_JOB);
                }

                break;
            }
            case ACK_JOB:
            {
                debug("ack job arrived from: %d museum: %d id: %d", packet.src, packet.museum_id, packet.id);
                
                // dwarf can't send ack for itself
                if (packet.src == rank) {
                    break;
                }

                pthread_mutex_lock(&queueJobsMut);

                if (jobs.front == -1 || jobs.rear == -1) {
                    break;
                }

                int on_duty = 0;

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

                if (on_duty && requests.front > -1 && requests.rear > -1) {
                    for (int j = jobs.front; j <= jobs.rear; j++) {
                        jobData *job = (jobData *) jobs.data[j];

                        packet_t *pkt = malloc(sizeof(packet_t));
                        pkt->museum_id = job->museum_id;
                        pkt->id = job->id;
                        pkt->request_ts = job->request_ts;
                        pkt->ack_count = job->ack_count;

                        int job_taken = 0;

                        for (int i = requests.front; i <= requests.rear; i++) {
                            request *req = (request *) requests.data[i];

                            debug("REQ %d %d %d | %d %d", req->dwarf_id, req->job->museum_id, job->museum_id, req->job->id, job->id);
                            
                            if (req->job->museum_id == job->museum_id &&
                                req->job->id == job->id &&
                                !job_taken) {
                                
                                // TODO: priorytetyzuje najpierw siebie
                                debug("Send TAKE to %d", req->dwarf_id);
                                sendPacket(pkt, req->dwarf_id, TAKE);
                                job_taken = 1;
                            }

                            if (job_taken) {
                                dequeueAt(&requests, i);
                            }
                            free(req);
                        }
                        free(pkt);
                    }
                }

                pthread_mutex_unlock(&queueJobsMut);
                break;
            }
            case TAKE:
            {   
                debug("take arrived from: %d museum: %d id: %d", packet.src, packet.museum_id, packet.id);

                pthread_mutex_lock(&queueJobsMut);

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

                sem_post(&jobAccessGranted);
                break;
            }
            default:
                break;
        }
    }

    return NULL;
}