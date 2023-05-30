#include "main.h"
#include "comm_thread.h"
#include "main_thread.h"


sem_t jobReserveMut;
pthread_t threadComm;

void check_thread_support(int provided);
void finalize();


int main(int argc, char **argv) {
    // MPI_Status status;
    int provided;

    sem_init(&jobReserveMut, 0, 0);

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

    srand(rank);

    init_packet_type();
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    changeState(generateJob);
    

    pthread_create(&threadComm, NULL, startCommThread, 0);

    mainLoop();

    finalize();

    return 0;
}

void check_thread_support(int provided) {
    printf("THREAD SUPPORT: we want: %d. What we get?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("No thread support, exitting\n");
	    fprintf(stderr, "Thread support not sufficient - exitting!\n");
	    MPI_Finalize();
	    exit(-1);
	    break;
        case MPI_THREAD_FUNNELED:
            printf("only threads, which executed mpi_init_thread can use mpi library\n");
	    break;
        case MPI_THREAD_SERIALIZED: 
            printf("only one thread can execute MPI commands simultaneously\n");
	    break;
        case MPI_THREAD_MULTIPLE: printf("PFull thread support\n"); /* tego chcemy. Wszystkie inne powodują problemy */
	    break;
        default: printf("No info\n");
    }
}

void finalize() {
    pthread_mutex_destroy(&stateMut);

    println("waiting for \"comm\" thread\n");
    pthread_join(threadComm, NULL);
    MPI_Type_free(&MPI_PACKET_T);
    MPI_Finalize();
}
