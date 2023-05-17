#include "main.h"
#include "main_thread.h"
#include "../queue.h"

void mainLoop() {
    srandom(rank);

    Queue jobs;
    initQueue(&jobs);

    debug("Main thread Museum start");

    while (state != inFinish) {
        switch (state) {
            case newJobArrived:

                break;
        }
    }
}
