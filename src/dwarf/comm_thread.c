#include "main.h"
#include "comm_thread.h"



void *startCommThread(void *ptr) {
    MPI_Status status;

    packet_t packet;

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
                    debug("new job data: %d src: %d ts: %d", packet.data, packet.src, packet.ts);
                    
                    pthread_mutex_lock(&queueJobsMut);

                    packet_t job;
                    job.data = packet.data;
                    job.src = packet.src;
                    job.ts = packet.ts;

                    enqueue(&jobs, &job);
                    pthread_cond_signal(&newJobReceived);
                    pthread_cond_wait(&newJobProcessed, &queueJobsMut);
                    pthread_mutex_unlock(&queueJobsMut);
                    break;
                }
            case REQ_JOB:
                {
                    debug("req job arrived: data: %d src: %d ts: %d", packet.data, packet.src, packet.ts);

                    int museum_id = packet.data & 0xFF;
                    int job_id = (packet.data >> 8) & 0xFFF;
                    int job_request_ts = (packet.data >> 20) & 0xFFF;
                    int job_exists = 0;

                    pthread_mutex_lock(&queueJobsMut);

                    for (int i = jobs.front; i <= jobs.rear; i++) {
                        packet_t *job = (packet_t *) jobs.data[i];

                        debug("%d %d | %d %d | %d %d", job->data, job_id, job->src, museum_id, job_request_ts, ((job->ts >> 16) & 0xFFFF));

                        if (job->data == job_id && 
                            job->src == museum_id) {
                            job_exists = 1;
                            if (job_request_ts < ((job->ts >> 16) & 0xFFFF)) {
                                 packet_t *pkt = malloc(sizeof(packet_t));

                                pkt->data = packet.data;
                                sendPacket(pkt, i, ACK_JOB);

                                free(pkt);
                                break;
                            }
                        }
                    }

                    if (job_exists == 0) {
                        sendPacket(&packet, packet.src, ACK_JOB);
                    }

                    pthread_mutex_unlock(&queueJobsMut);

                    break;
                }
            case ACK_JOB:
                {
                    debug("ack job arrived: data: %d src: %d ts: %d", packet.data, packet.src, packet.ts);

                    break;
                }
            default:
                break;
        }
    }

    return NULL;
}