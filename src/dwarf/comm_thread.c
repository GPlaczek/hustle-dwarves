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
                
                pthread_mutex_lock(&queueJobsMut);
                packet_t pkt = packet;

                enqueue(&packets, &pkt);

                pthread_cond_signal(&newJobReceived);
                pthread_cond_wait(&newJobProcessed, &queueJobsMut);
                pthread_mutex_unlock(&queueJobsMut);
                break;
            }
            case REQ_JOB:
            {
                debug("req job arrived: museum: %d request_ts: %d id: %d", packet.museum_id, packet.request_ts, packet.id);

                int job_exists = 0;

                pthread_mutex_lock(&queueJobsMut);

                debug("%d %d", packets.front, packets.rear);

                if (packets.front < 0 || packets.rear < 0) {
                    sendPacket(&packet, packet.src, ACK_JOB);
                    break;
                }

                for (int i = packets.front; i <= packets.rear; i++) {
                    packet_t *job = (packet_t *) packets.data[i];

                    debug("%d", i);
                    debug("%d %d | %d %d | %d %d", job->id, packet.id, job->museum_id, packet.museum_id, job->request_ts, packet.request_ts);

                    if (job->id == packet.id && 
                        job->museum_id == packet.museum_id) {
                        job_exists = 1;
                        if (job->request_ts < packet.request_ts) {
                                packet_t *pkt = malloc(sizeof(packet_t));

                            debug("CHUJ");
                            pkt = &packet;
                            sendPacket(pkt, i, ACK_JOB);

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
                debug("ack job arrived: %d museum: %d id: %d", packet.src, packet.museum_id, packet.id);

                break;
            }
            default:
                break;
        }
    }

    return NULL;
}