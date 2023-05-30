#include "main.h"
#include "main_thread.h"
#include "../list.h"

void mainLoop() {
    srandom(rank);
    
    List packets;

    initList(&packets);

    debug("Main thread Museum start");

    while (state != inFinish) {
        switch (state) {
            case generateJob:
            {
                packet_t packet;

                packet.museum_id = rank;
                packet.id = lamport_time;
                packet.request_ts = 0;
                packet.ack_count = 0;

                debug("Generated new job: %d", packet.id);
                addNode(&packets, &packet);
                changeState(sendNewJob);
                debug("New State: %d\n", state);
                sleep(rand() % 2 + 1);
                break;
            }
            case sendNewJob: 
            {
                packet_t *packet = (packet_t *) packets.head->data;

                packet_t *pkt = malloc(sizeof(packet_t));
                pkt->id = packet->id;
                pkt->ack_count = packet->ack_count;
                pkt->museum_id = packet->museum_id;
                pkt->request_ts = packet->request_ts;

                debug("new %d %d %d %d", pkt->museum_id, pkt->id, pkt->request_ts, pkt->ack_count);

                for (int i = 0; i < size; i++) {
                    if (i != rank) {
                        sendPacket(pkt, i, NEW_JOB);
                    }
                }
                free(pkt);
                changeState(waitForReserve);
                break;
            }
            case waitForReserve:
            {
                packet_t *pkt = (packet_t *) packets.head->data;

                debug("Wait for reserve: %d", pkt->id);

                sem_wait(&jobReserveMut);
                changeState(generateJob);
                removeNode(&packets, pkt);
                break;
            }
            default:
                break;
        }
    }

    debug("main thread finish\n");
}
