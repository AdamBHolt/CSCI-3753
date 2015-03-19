#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "util.h"
#include "queue.h"

#define USAGE "<inputFilePath> <outputFilePath>"
#define MINARGS 3
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define QSIZE 10

//Function prototypes
void* requester(void*);
void* resolver(void*);

//Struct to pass data into threads
struct thread_info {
    FILE* thread_file;
    pthread_mutex_t* buffer_mutex;
    pthread_mutex_t* outfile_mutex;
    queue* buffer;
};