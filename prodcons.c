/*
 * Deirdre Sweeney
 * CS 1550: Introduction to Operating Systems
 * Professor Misurda
 * Project 2
 * 10/05/14
 * 
 * Producer/Consumer model using semaphores
 * 
 * prodcons.c
 */

// INCLUDES
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <math.h>
#include <sys/types.h>
#include "queue.h"

#define BUFFSIZE 1000
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

// DEFINITIONS
/* semaphore struct with value and queue implementation */
struct cs1550_sem
{
	int value;
	struct task_struct * queue_array[100]; 
	struct task_struct * item;
	int rear;
	int front;
};

/* struct of shared values within prodcons */
typedef struct {
	int buff_size;
	int buffer[BUFFSIZE];
	struct cs1550_sem full;
	struct cs1550_sem empty;
	struct cs1550_sem mutex;
	int in;
	int out;
} shared_data;

// PROTOTYPES
void producer(int index);
void consumer(int index);
int create_process(int index, void (*pFunc)());
void down(struct cs1550_sem * sem);
void up(struct cs1550_sem * sem);

shared_data * shared;

int main(int  argc, char** argv)
{
	if (argc != 4) 
	{
		printf("Usage: the number of consumers and producers specified on the command line followed by the size of the buffer. \n"
				"For example: ./prodcons 2 2 1000 \n");
		return -1;
	}

	if (atoi(argv[3]) > 1000)
	{
		printf("Size of buffer is too high. Maximum of buffer size is currently set to 1000.\n"
				"\t To increase buffer size, change '#define BUFFSIZE 1000' at line 23 of prodcons.c");
		return -1;
	}

	int num_producers   = atoi(argv[1]);
	int num_consumers   = atoi(argv[2]);

	int index;
	void (*producerPtr)(int);
	void (*consumerPtr)(int);
	producerPtr = &producer;
	consumerPtr = &consumer;

	// Use mmap() to get space for shared variables and initialize all constants.
	shared = (shared_data *) mmap(NULL, sizeof(shared_data), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	shared->buff_size 	= atoi(argv[3]);
	shared->empty.value = shared->buff_size;
	shared->empty.front = -1;
	shared->empty.rear = -1;
	shared->full.value = 0;
	shared->full.front = -1;
	shared->full.rear = -1;
	shared->mutex.value = 1;
	shared->mutex.front = -1;
	shared->mutex.rear = -1;

	shared->in = 0;
	shared->out = 0;

	// create process for producer and consumer
	for (index = 0; index < MAX(num_producers, num_consumers); index++) 
	{
		if (index < num_producers)
			create_process(index, producerPtr);
		if (index < num_consumers)
			create_process(index, consumerPtr);
	}
}

void producer(int index)
{
	int pitem;

	while(1)
	{
		//call down() on empty and mutex semaphores 
		down(&shared->empty);
		down(&shared->mutex);

		// items added are consequtive numbers
		pitem = shared->in;
		// add item to shared buffer
		((int *)shared->buffer)[shared->in] = pitem;
		// increase number in buffer
		shared->in = (shared->in+1) % shared->buff_size;

		printf("Producer %c produced: %d\n", index + 'A', pitem);
		sleep(5); /* easy to read output */

		// call up() on mutex and full semaphores
		up(&shared->mutex);
		up(&shared->full);
	}
	exit(0);
}

void consumer(int index)
{
	int citem;

	while(1)
	{
		// call down() on full and mutex semaphores
		down(&shared->full);
		down(&shared->mutex);

		// get count from buffer
		citem = ((int *)shared->buffer)[shared->out];
		// increase number out of buffer
		shared->out = (shared->out+1) % shared->buff_size;

		printf("Consumer %c consumed: %d\n", index + 'A', citem);
		sleep(5);

		// call up() on mutex and empty semaphores
		up(&shared->mutex);
		up(&shared->empty);
	}
	exit(0);
}

// private method to create process. 
int create_process(int index, void (*pFunc)())
{
	int pid;

	// if child process, call function pointer.
	if ((pid = fork()) == 0)
	{
		(*pFunc)(index);
		exit(0);
	}
	return pid;
}

// wrapper classes for down() and up()
void down(struct cs1550_sem * sem) 
{
    syscall(__NR_cs1550_down, sem);
}

void up(struct cs1550_sem *sem) 
{
	syscall(__NR_cs1550_up, sem);
}




