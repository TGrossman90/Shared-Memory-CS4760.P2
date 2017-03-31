//Tom Grossman
//CS4760 - Operating Systems
//Project 2 - Shared Memory
//03/14/17
//Copyright © 2017 Tom Grossman. All rights reserved.

#ifndef PROJECT2_H
#define PROJECT2_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>

int slaveChecker(pid_t [], int, int);

void signalHandler();
void killAll();
void printHelp();

struct SharedMemory {
	int sharedVar;
	int turn;
	int flag[19];
	int maxWrites;
	int slaveProcesses;
	int processes;
};

enum state {idle, want_in, in_cs};

key_t key = 1337;

#endif