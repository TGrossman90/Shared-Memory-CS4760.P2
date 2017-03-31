//Tom Grossman
//CS4760 - Operating Systems
//Project 2 - Shared Memory
//03/14/17
//Copyright © 2017 Tom Grossman. All rights reserved.

#include "project2.h"

int shmid;
struct SharedMemory *shm;

int main(int argc, char* argv[]) {
	// Signal handler
	signal(SIGINT, signalHandler);

	// Defaults and slave target path
	char *fileName = "test.out";
	const char *PATH = "./slave";

	int op, totalProcesses,
		index = 0,
		maxWrites = 3,
		slaveProcesses = 5,
		terminationTime = 20;

	// Taken from https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt
	// Modified to fit what I needed
	while ((op = getopt (argc, argv, "hs:l:i:t:")) != -1) {
		switch (op) {
			case 'h':
				printHelp();
				exit(EXIT_SUCCESS);
				
			case 's':
				if(isdigit(*optarg)) {
					if(atoi(optarg) < 20) {
						slaveProcesses = atoi(optarg);
					} else {
						printf("Cannot spawn more than 20 processes (master process counts as 1). Please try again with a number between 1-19... defaulting to 5 slaves\n");
						exit(EXIT_FAILURE);
					}
				} else {
					fprintf(stderr, "'-s' expects an integer value\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
				
			case 'l':
				fileName = optarg;
				break;
				
			case 'i':
				if(isdigit(*optarg)) {
					maxWrites = atoi(optarg);
				} else {
					fprintf(stderr, "'-i' expects an integer value\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
				
			case 't':
				if(isdigit(*optarg)) {
					terminationTime = atoi(optarg);
				} else {
					fprintf(stderr, "'-t' expects an integer value\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
				
			case '?':
				if(optopt == 's' || optopt == 'l' || optopt == 'i' || optopt == 't') {
					fprintf(stderr, "-%c requires an argument!\n", optopt);
				} else if(isprint(optopt)) {
					fprintf(stderr, "-%c is an unknown flag\n", optopt);
				} else {
					fprintf(stderr, "%s is unknown\n", argv[optind - 1]);
				}
				
			default:
				printf("-%c is not an option.\n", optarg);
				printHelp();
		}
	}

	// Create shared memory id
	if((shmid = shmget(key, sizeof(struct SharedMemory) * 2, IPC_CREAT | 0666)) < 0) {
		perror("shmget");
		fprintf(stderr, "shmget() returned an error! Program terminating...\n");
		exit(EXIT_FAILURE);
	}
	
	// Attach shared memory
    if ((shm = (struct SharedMemory *)shmat(shmid, NULL, 0)) == (struct SharedMemory *) -1) {
		perror("shmat");
        fprintf(stderr, "shmat() returned an error! Program terminating...\n");
        exit(EXIT_FAILURE); 
    }
	
	// Time 2 - Time 1 = Seconds that have past
	time_t t1 = time(NULL);
	time_t t2 = time(NULL); 
	
	// Process ID's
	pid_t pid, slaves[slaveProcesses];
	
	// Save data to shared memory
	shm->maxWrites = maxWrites;
	shm->sharedVar = 0;
	shm->slaveProcesses = slaveProcesses + 1;
	shm->turn = 0;

	// Begin master process
	//Terminate after terminationTime elapses
	while((t2 - t1) < terminationTime) {
		if(totalProcesses < 20) {
			// Only fork the number of processes specified
			if(index < slaveProcesses) {
				pid = fork();
				slaves[index] = pid;
				totalProcesses++;
				index++;
			}
			
			if(pid == -1) {
				fprintf(stderr, "Fork Error! Terminating...\n");
				killAll();
				exit(EXIT_FAILURE);
			
			// Spawn slave process 
			} else if(pid == 0) {
				char *id;
				sprintf(id, "%i", index);
				shm->processes++;
				
				//printf("Creating Slave Process\n");
				//https://linux.die.net/man/3/execl
				//execl(PATH, argv[0], argv[1], argv[2], argv[3])
				execl(PATH, id, fileName, (char *)NULL);

				//If child program exec fails, _exit()
				_exit(EXIT_FAILURE);

			}
			
			//Reset time to know how much time has passed
			t2 = time(NULL);					
		
		// If too many processes, terminate
		} else {
			fprintf(stderr, "There are too many processes running! Killing all and exiting...\n");
			signalHandler(2);
			exit(EXIT_FAILURE);
		}
		
		// Check slaves for status
		// If any slaves are active continue,
		// else all slaves are dead and master can terminate
		int check = slaveChecker(slaves, slaveProcesses, maxWrites);
		if(check == 1) {
			break;
		}
			
	}

	// If termination time passes, check if slaves are still running
	// If still running, kill all processes and return error'
	// Else terminate successfully
	if((wait(NULL) > 0) && (shm->processes != 0)) {
		fprintf(stderr,"Error: Did not complete all processes before termination time limit reached! Terminating all processes...\n");
		signalHandler();
	} else {
		killAll();
		printf("Completed %i writes for %i processes!\n", maxWrites, slaveProcesses);
	}
}

// Checks if slaves are running
int slaveChecker(pid_t slaves[], int size, int maxWrites) {
	int c, status;
	
	for(c = 0; c < size; c++) {
		pid_t wid;
		wid = waitpid(slaves[c], &status, WNOHANG);
		
		if(wid != 0) {
			slaves[c] = 0;
		}
	}
	
	for(c = 0; c < size; c++) {
		if(slaves[c] == 0)
			continue;
		else
			return 0;
	}	
		return 1;
}

void printHelp() {
	printf("\nCS4760 Project 2 Help!\n");
	printf("-h <prints help>\n");
	printf("-s [int x] <spawn x slave processes>\n");
	printf("-l [string fileName] <changes export logfile name>\n");
	printf("-i [int x] <change number of slave process writes to file>\n");
	printf("-t [int x] <change termination time limit>\n\n");
}

// Kills all when signal is received
void signalHandler() {
    pid_t id = getpgrp();
	printf("Signal received... terminating master with PID: %i\n", id);
	killAll();
    killpg(id, SIGINT);
    exit(EXIT_SUCCESS);
}

// Release shared memory
void killAll() {
	shmctl(shmid, IPC_RMID, NULL);
	shmdt(shm);
}
