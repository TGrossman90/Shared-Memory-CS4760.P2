//Tom Grossman
//CS4760 - Operating Systems
//Project 2 - Shared Memory
//03/14/17
//Copyright © 2017 Tom Grossman. All rights reserved.

#include "project2.h"

int shmid;
struct SharedMemory *shm;
char *fileName;

int main(int argc, char* argv[]) {
	// Signal Handler
	signal(SIGINT, signalHandler);	
	
	// Get passed arguments from execl()
	int i = atoi(argv[0]);	
	fileName = argv[1];
	
	
	// Get shared memory id that was created by the master process	
	if((shmid = shmget(key, sizeof(struct SharedMemory) * 2, 0666)) < 0) {
		perror("shmget");
		fprintf(stderr, "shmget() returned an error! Program terminating...\n");
		exit(EXIT_FAILURE);
	}
	
	// Attach the shared memory to the slave process
    if ((shm = (struct SharedMemory *)shmat(shmid, NULL, 0)) == (struct SharedMemory *) -1) {
		perror("shmat");
        fprintf(stderr, "shmat() returned an error! Program terminating...\n");
        exit(EXIT_FAILURE);
    }
	
	// Peterson's Algorithm for more than two processes (slides)
	int c, j, n = shm->slaveProcesses;
	for(c = 0; c < shm->maxWrites; c++) {
		do {
			shm->flag[i] = want_in;
			
			//Set local variable
			j = shm->turn; 
			
			while(j != i) {
				j = (shm->flag[j] != idle) ? shm->turn : (j + 1) % n;
			}
			
			// Declare intention to enter critical section
			shm->flag[i] = in_cs;
			
			// Check that no one else is in critical section
			for(j = 0; j < n; j++) {
				if((j != i) && (shm->flag[j] == in_cs)) {
					break;
				}
			}
		} while((j < n) || ((shm->turn != i) && (shm->flag[shm->turn] != idle)));
		
		// Assign to self and enter critical section
		shm->turn = i; 
		
		// **Critical Section**
		fprintf(stderr, "Child PID: %i - Entering Critical Section\n", getpid());
		
		// Open a file to output log messages to
		FILE *fp;
		fp = fopen(fileName, "a");
		
		// If file can't open, exit_failure
		if(fp == NULL) {
			printf("File could not be opened!\n");
			errno = ENOENT;
			killAll();
			exit(EXIT_FAILURE);
		}
		
		// Generate a random number so we can pause for 0-2 seconds
		// This seed generation was fount at: http://stackoverflow.com/questions/36230140/c-rand-function-doesnt-work-properly-with-sleep-function
		// Each slave process will generate distinct rand() numbers because
		// all processes srand() seed is different
		struct timespec tm;
		clock_gettime(CLOCK_MONOTONIC, &tm);
		srand((unsigned)(tm.tv_sec ^ tm.tv_nsec ^ (tm.tv_nsec >> 31)));
		sleep((rand() % 3));
		
		// Get the localtime in the format HOURS:MINUTES:SECONDS for the log message
		char *t = malloc(10);
		time_t theTime;
		theTime = time(NULL);
		struct tm *p = localtime(&theTime);
		strftime(t, 10, "%H:%M:%S", p);
		
		// Increment shared variable
		shm->sharedVar++;
		
		// Print to screen the log message and write it to the file
		printf("\tFile modified by process number %i at time %s with sharedNum = %d\n", i, t, shm->sharedVar);
		fprintf(fp, "File modified by process number %i at time %s with sharedNum = %d\n", i, t, shm->sharedVar);
		fclose(fp);
		
		// Sleep again for 0-2 seconds
		sleep((rand() % 3));
		
		// **Exit Critical Section**
		j = (shm->turn + 1) % n;
		while(shm->flag[j] == idle) {
			j = (j + 1) % n;
		}
		fprintf(stderr, "\t\tChild PID: %i - Exiting Critical Section\n\n", getpid());
		
		// Assign turn to next waiting process; change own flag to idle
		shm->turn = j;
		shm->flag[i] = idle;
	}
	
	
	// Cleanup and Exit
	shm->processes--;
	killAll();
	exit(3);
}

// Kills all when signal is received
void signalHandler() {
    pid_t id = getpid();
	printf("Signal received... terminating slave with PID: %i\n", id);
	killAll();
    killpg(id, SIGINT);
    exit(EXIT_SUCCESS);
}

// Release shared memory
void killAll() {
	shmdt(shm);
}
