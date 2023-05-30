#include "main.h"
#include "comm_thread.h"
#include "main_thread.h"

// int rank, size;
// int lamport_time = 0;

pthread_t threadComm;
pthread_mutex_t queueJobsMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queuePortalsMut = PTHREAD_MUTEX_INITIALIZER;

sem_t jobAccessGranted;
sem_t waitNewJobSem;
sem_t waitForJobProcessed;
sem_t waitForPortalAccess;
List jobs;
List portals;



void check_thread_support(int provided);
void finalize();


int main(int argc, char **argv) {
    // MPI_Status status;
    int provided;

    sem_init(&waitNewJobSem, 0, 0);
    sem_init(&waitForJobProcessed, 0, 0);
    sem_init(&jobAccessGranted, 0, 0);
    sem_init(&waitForPortalAccess, 0, 0);

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

    srand(rank);

    init_packet_type();
    initList(&jobs);
    initList(&portals);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    changeState(waitForNewJob);

    pthread_create(&threadComm, NULL, startCommThread, 0);

    mainLoop();

    finalize();

    return 0;
}

void check_thread_support(int provided) {
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("Brak wsparcia dla wątków, kończę\n");
            /* Nie ma co, trzeba wychodzić */
	    fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
	    MPI_Finalize();
	    exit(-1);
	    break;
        case MPI_THREAD_FUNNELED: 
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	    break;
        case MPI_THREAD_SERIALIZED: 
            /* Potrzebne zamki wokół wywołań biblioteki MPI */
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	    break;
        case MPI_THREAD_MULTIPLE: printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
	    break;
        default: printf("Nikt nic nie wie\n");
    }
}

void finalize() {
    pthread_mutex_destroy(&stateMut);

    println("waiting for \"comm\" thread\n");
    pthread_join(threadComm, NULL);
    MPI_Type_free(&MPI_PACKET_T);
    MPI_Finalize();
}
