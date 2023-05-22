#include "main.h"
#include "main_thread.h"
#include "../queue.h"

void mainLoop() {
    srandom(rank);
    
    Queue packets;
    initQueue(&packets);

    debug("Main thread Museum start");

    while (state != inFinish) {
        switch (state) {
            case generateJob:
            {
                packet_t packet;

                packet.museum_id = rank;
                packet.id = lamport_time + 69;
                packet.request_ts = -1;
                packet.ack_count = 0;

                debug("Generated new job: %d", packet.id);
                enqueue(&packets, &packet);
                changeState(sendNewJob);
                debug("New State: %d\n", state);
                sleep(rand() % 2 + 1);
                break;
            }
            case sendNewJob: 
            {
                packet_t *packet = (packet_t *) packets.data[packets.front];

                packet_t *pkt = malloc(sizeof(packet_t));
                pkt = packet;

                debug("new %d %d %d %d", pkt->museum_id, pkt->id, pkt->request_ts, pkt->ack_count);

                for (int i = 0; i < size; i++) {
                    if (i != rank) {
                        sendPacket(pkt, i, NEW_JOB);
                    }
                }
                sem_wait(&jobReserveMut);
                changeState(waitForReserve);
                break;
            }
            case waitForReserve:
            {
                packet_t *pkt = (packet_t *) packets.data[packets.front];

                debug("Wait for reserve: %d", pkt->id);

                sem_wait(&jobReserveMut);
                changeState(generateJob);
                dequeue(&packets);
                break;
            }
            default:
                break;
        }
    }

    debug("main thread finish\n");
}
