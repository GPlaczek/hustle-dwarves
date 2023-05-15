#include "main.h"
#include "comm_thread.h"

void *startCommThread(void *ptr) {
    MPI_Status status;

    int is_message = FALSE;
    packet_t packet;

    int clock = 0;

    debug("Comm thread Dwarf start");
}