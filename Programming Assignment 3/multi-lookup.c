/*
 * File: multi-lookup.c
 * Authors: Joe Cosenza, Adam Holt
 *  
 */

#include "multi-lookup.h"

int requester_counter = 1;

int main(int argc, char * argv[]){

	//Check for correct number of arguments
	if(argc<MINARGS){
		fprintf(stderr, "Not enough arguments: %d\n", argc-1);
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}
	
	//Local variable declarations
	//Possible error codes
	int rc;
	char errorstr[SBUFSIZE];
	int mutex_error;
	//Number of cores on the current system
	int numCPU = sysconf( _SC_NPROCESSORS_ONLN );
	//Set number of resolver threads
	int resolver_threads = numCPU < MAX_RESOLVER_THREADS ? numCPU : MAX_RESOLVER_THREADS;
	resolver_threads = resolver_threads < MIN_RESOLVER_THREADS ? MIN_RESOLVER_THREADS : resolver_threads;
	//Counters
	int t, i;
	//Queue to hold hostnames
	queue buffer;    
	//Number of input files from command line
	int input_files = argc-2;
	//Array of file pointers for input
	FILE* infiles[input_files];
	//Output file pointer
	FILE* outfile = NULL;
	//Arrays of pthreads for requesters and resolvers
	pthread_t requesters[input_files];
	pthread_t resolvers[resolver_threads];
	//Arrays of mutexes for queue and output file
	pthread_mutex_t buffer_mutex;
	pthread_mutex_t outfile_mutex;
	//Structs to pass data to threads as they are created
	struct thread_info requester_info[input_files];
	struct thread_info resolver_info[resolver_threads];

	printf("Number of input files: %d\n%d requester threads created.\n", input_files, input_files);
	printf("Number of cores detected: %d\n%d resolver threads created.\n\n", numCPU, resolver_threads);
	
	//Ensure that the maximum number of input files is 10
	if((input_files)>MAX_INPUT_FILES){
		fprintf(stderr, "Maximum of 10 input files allowed.\n");
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}
	
	//Open output file
	outfile = fopen(argv[argc-1], "w");
	if(!outfile){
		perror("Error opening output file");
		return EXIT_FAILURE;
	}

	//Open input files
	for(i=1; i<(argc-1); i++){
		infiles[i-1] = fopen(argv[i], "r");
		if(!infiles[i-1]){
			sprintf(errorstr, "Error opening file: %s", argv[i]);
			perror(errorstr);
			break;
		}
	}
	
	//Initialize buffer queue
	if(queue_init(&buffer, QSIZE) == QUEUE_FAILURE)
        fprintf(stderr, "ERROR: queue_init failed!\n");
    
	//Initialize mutexes
	mutex_error = pthread_mutex_init(&buffer_mutex, NULL);
	if(mutex_error)
		fprintf(stderr,"ERROR: Return code from buffer_mutex is %d\n", mutex_error);

	mutex_error = pthread_mutex_init(&outfile_mutex, NULL);
	if(mutex_error)
		fprintf(stderr,"Error: Return code from outfile_mutex is %d\n", mutex_error);

	//Create requester pthreads for each input file
	for(t=0; t<input_files; t++){
		//Set data for struct to pass to current pthread
		requester_info[t].thread_file = infiles[t];
		requester_info[t].buffer_mutex = &buffer_mutex;
		requester_info[t].outfile_mutex = NULL;
		requester_info[t].buffer = &buffer;		

		//Attempt to create the pthread
		rc = pthread_create(&(requesters[t]), NULL, requester, &(requester_info[t]));
		if(rc){
			printf("ERROR: return code from pthread_create() is %d\n", rc);
            		exit(EXIT_FAILURE);
        	}
		printf("Created requester thread %d\n", t);
    }
    
	//Create resolver pthreads
    for(t=0; t<resolver_threads; t++){
       	resolver_info[t].thread_file = outfile;
		resolver_info[t].buffer_mutex = &buffer_mutex;
		resolver_info[t].outfile_mutex = &outfile_mutex;
		resolver_info[t].buffer = &buffer;

       	rc = pthread_create(&(resolvers[t]), NULL, resolver, &(resolver_info[t]));
       	if(rc){
           		printf("ERROR: return code from pthread_create() is %d\n", rc);
           		exit(EXIT_FAILURE);
     	}
		printf("Created resolver thread %d\n", t);
    }

    //Wait for requester threads to complete
    for(t=0; t<input_files; t++){
        pthread_join(requesters[t], NULL);
		printf("Requester thread %d finished\n", t);
	}
	
	//Indicate that no more requesters are running
	requester_counter=0;
	
    //Wait for resolver threads to complete
    for(t=0; t<resolver_threads; t++){
        pthread_join(resolvers[t], NULL);
		printf("Resolver thread %d finished\n", t);
	}
	//Clean queue
	queue_cleanup(&buffer);

	//Close output file
	if(fclose(outfile)){
		perror("Error Closing Output File");
		return EXIT_FAILURE;
    }
	
	//Close input files
	for(i=0; i<input_files; i++){
		if(fclose(infiles[i])){
			perror("Error Closing Input File");
		}
	}
	
	//Destroy the mutexes
	pthread_mutex_destroy(&buffer_mutex);
	pthread_mutex_destroy(&outfile_mutex);

    return 0;
}

void* requester(void* threadid){
  
	//Struct to hold thread-specific information 
	struct thread_info* thread_info = threadid;
	//Error codes for lock/unlock procedures
	int lock_error, unlock_error;
	//Hostname char arrays
   	char hostname[SBUFSIZE];
	char *host;
	//Input file for this thread
	FILE* infile = thread_info->thread_file;
	//Buffer mutex
	pthread_mutex_t* buffer_mutex = thread_info->buffer_mutex;
	//Queue to write to
	queue* buffer = thread_info->buffer;
	int pushed = 0;

   	//Read the current input file and push its entries onto the buffer
   	while(fscanf(infile, INPUTFS, hostname) > 0){
		
		//Repeat until the current hostname is pushed to the queue
		while(!pushed){
			host = malloc(SBUFSIZE);
			strncpy(host, hostname, SBUFSIZE);
    
			//Lock the buffer
			lock_error = pthread_mutex_lock(buffer_mutex);
			if(lock_error)
				fprintf(stderr, "ERROR: pthread_mutex_lock() code: %d\n", lock_error);
	
			//Wait until the buffer is not full - spinlock while waiting
			while(queue_is_full(buffer)){
					//Unlock the buffer
					unlock_error = pthread_mutex_unlock(buffer_mutex);
					if(unlock_error)
								fprintf(stderr, "ERROR: pthread_mutex_unlock() code: %d\n", unlock_error);
					//Wait 0-100 microseconds
					usleep(rand() % 100 + 1);
					//Lock the buffer
					lock_error = pthread_mutex_lock(buffer_mutex);
					if(lock_error)
						fprintf(stderr, "ERROR: pthread_mutex_lock() code: %d\n", lock_error);
			}
	
			//when the buffer is not full, push the current hostname onto the buffer
        	if(queue_push(buffer, host) == QUEUE_FAILURE)
            		fprintf(stderr, "ERROR: queue_push failed!\n");

			//Unlock the buffer
			unlock_error = pthread_mutex_unlock(buffer_mutex);
			if(unlock_error)
				fprintf(stderr, "ERROR: pthread_mutex_unlock() code: %d\n", unlock_error);
			
			//Indicate that this hostname was pushed successfully
			pushed = 1;
    	}
		//Reset pushed for the next hostname
		pushed = 0;
	}
	
    return NULL;
}

void* resolver(void* threadid){
	//Struct for thread-specific information
    struct thread_info* thread_info = threadid;
	//File to write to
    FILE* outfile = thread_info->thread_file;
	//Buffer mutex
	pthread_mutex_t* buffer_mutex = thread_info->buffer_mutex;
	//Output file mutex
	pthread_mutex_t* outfile_mutex = thread_info->outfile_mutex;
	//Buffer to read from 
	queue* buffer = thread_info->buffer;	
	//Resolved IP address strings 
 	char ipstr[INET6_ADDRSTRLEN];   
	char* hostname;
	//Error codes for mutex lock/unlock operations
	int lock_error, unlock_error;

	//Repeat while the buffer is not empty or there are still running requester threads
    while(!queue_is_empty(buffer) || requester_counter){
		//Lock the buffer
		lock_error = pthread_mutex_lock(buffer_mutex);
		if(lock_error)
			fprintf(stderr, "ERROR: pthread_mutex_lock() code: %d\n", lock_error);
		
		//Pop the next entry off the queue
		hostname = queue_pop(buffer);
		
		//If nothing was popped off the queue
		if(hostname == NULL){
			//Unlock the buffer
			unlock_error = pthread_mutex_unlock(buffer_mutex);
			if(unlock_error)
				fprintf(stderr, "ERROR: pthread_mutex_unlock() code: %d\n", unlock_error);	
			//Wait 0-100 microseconds
			usleep(rand() % 100 + 1);
		}		
		else {
			//Unlock the buffer
			unlock_error = pthread_mutex_unlock(buffer_mutex);
			if(unlock_error)
				fprintf(stderr, "ERROR: pthread_mutex_unlock() code: %d\n", unlock_error);	
		
			//Lookup hostname and get IP string
			if(dnslookup(hostname, ipstr, sizeof(ipstr)) == UTIL_FAILURE){
				fprintf(stderr, "dnslookup error: %s\n", hostname);
				strncpy(ipstr, "", sizeof(ipstr));
			}
			
			//Write to the output file
			//Lock the output file
            lock_error = pthread_mutex_lock(outfile_mutex);
				if(lock_error)
					fprintf(stderr, "Error: pthread_mutex_lock() code: %d\n", lock_error);
			
			//Print the host and ip address to the output file
			fprintf(outfile, "%s,%s\n", hostname, ipstr);	

			//Unlock the output file
			unlock_error = pthread_mutex_unlock(outfile_mutex);
			if(unlock_error)
				fprintf(stderr, "ERROR: pthread_mutex_unlock() code: %d\n", unlock_error);	
			
    	}
			free(hostname);
	}
    return NULL;
}