#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#define N 10
typedef struct Customer {
	int id;
	struct Customer* next;
}Customer, * pCustomer;

pCustomer StandList = NULL, SitList = NULL; // linked list to standing & sitting
sem_t full, mutex, sit, pay, payment, care, confirm, ready;

int inClinic = 0;
pCustomer Add(pCustomer p, int id);// / Add a Customer to the list
pCustomer MovePlace(pCustomer p, int* id); // transfer places between list
void* customerF(void* pid);// Customer function
void* dhF(void* nid);// Descaling dh function.

void main() {
	/*
	Main does not receive anything or return anything. Main intializes all the semaphores and using loops calls upon threads.
	At the end main joins all the threads.
	*/
	pthread_t pThread[N + 2], nThread[3];
	int i, pID[N + 2], nID[3];
	sem_init(&mutex, 0, 1);
    // Customer
	sem_init(&sit, 0, 4);
	sem_init(&care, 0, 3);
	sem_init(&pay, 0, 0);
	// dhs
	sem_init(&ready, 0, 0); 
	sem_init(&confirm, 0, 0); 
	sem_init(&payment, 0, 0);	// Clinic
	sem_init(&full, 0, 0);	// Creating threads.
	for (i = 0; i < N + 2; i++) {
		pID[i] = i + 1;
		if (pthread_create(&pThread[i], NULL, customerF, (void*)&pID[i]) != 0) {
			perror("Error: Creating thread.\n");
			exit(1);
		}
	}
    for (i = 0; i < 3; i++) {
		nID[i] = i + 1;
		if (pthread_create(&nThread[i], NULL, dhF, (void*)&nID[i]) != 0) {
			perror("Error: Creating thread.\n");
			exit(1);
		}
}
	// Join to all threads.
	for (i = 0; i < N + 2; i++)
		pthread_join(pThread[i], NULL);
	for (i = 0; i < 3; i++)
		pthread_join(nThread[i], NULL);
}

pCustomer Add(pCustomer pH, int id)
{
	/*
	Function receives a pointer to struct Customer and an int. Function returns a pointer to struct Customer.
	Function allocates memory in order to add to the linked list.
	*/
	pCustomer p = (pCustomer)malloc(sizeof(Customer));
	if (!p)
	{
		printf("Error: Memory allocation failed.\n");
		exit(1);
	}	//init Customer
	p->id = id;
	p->next = NULL;
	if (pH == NULL) // if is the first Customer in the list.
	{
		pH = p;
	}
    	else { // else add to the end of the list
		pCustomer r = pH;
		while (r->next != NULL) r = r->next;
		r->next = p;
	}
	return pH;
}
pCustomer MovePlace(pCustomer p, int* id)
{
	/*
	Function receives a pointer to struct Customer and a pointer to an int. Function returns a pointer to struct Customer.
	Function frees the struct up in the linked list and moves up the linked next struct.
	*/
	if (p == NULL)
		return p;
	pCustomer temp = p;
	*id = p->id; // save the thread id
	p = p->next;
	free(temp);
	return p;
}
void* customerF(void* pid)
{
	/*
	Function receives a void pointer which it turns into an int. Function does not return anything.
	Function uses the semaphores to run the customer through the clinic as requested in hw.
	*/
	int* id = (int*)pid;
	while (1)
	{
		sem_wait(&mutex);
		if (inClinic < N)
        {
			sem_post(&mutex); 
			sem_wait(&mutex); 
			StandList = Add(StandList, *id); // Add Customer to standing list.
			sem_post(&mutex); 
			// Get in
			sem_wait(&mutex); 
			inClinic++; // update counter
			sem_post(&mutex);
			printf("I'm Customer #%d, I got into the clinic.\n", *id);
			sleep(1);
			// Sit
			sem_wait(&sit); 
			sem_wait(&mutex); 
			StandList = MovePlace(StandList, id); // From standing to sitting
			sem_post(&mutex); 
			printf("I'm Customer #%d, I'm sitting on the sofa.\n", *id);
			SitList = Add(SitList, *id); // Add Customer to sitting list.
			sleep(1);
            // Get care
			sem_wait(&care);
			sem_wait(&mutex);
			SitList = MovePlace(SitList, id); // From sitting to caring
			sem_post(&mutex); 
			printf("I'm Customer #%d, I'm getting treatment.\n", *id);
			sleep(1);
			sem_post(&sit); 
			sem_post(&ready);
            // Ready to treatment.
			sem_wait(&pay); // ready to pay
			printf("I'm Customer #%d, I'm paying now.\n", *id);
			sem_post(&payment); // Paying
			sem_wait(&confirm); // Waiting to confirm from dh
			sleep(1);
		}
        else {
			// Clinic full
			sem_post(&mutex);
			printf("I'm Customer #%d, I'm out of clinic.\n", *id);
			sleep(1);
			sem_wait(&full);
		}
	}
}
void* dhF(void* nid)
{
	/*
	Function receives a void pointer which it turns into an int. Function does not return anything.
	Function is the dental hygienist part in letting customers through the clinic as requested in hw.
	*/
	int* id = (int*)nid;
	while (1)
	{
		sem_wait(&ready); // Ready to a Customer
		printf("I'm Dental Hygienist #%d, I'm working now.\n", *id);
		sem_post(&pay);// Treatment finished.
		sleep(1);

		sem_wait(&payment); // Waiting to payment.
		printf("I'm Dental Hygienist #%d, I'm getting a payment.\n", *id);
	
		sem_wait(&mutex);	// Customer exit
		inClinic--; //Customer was left
		sem_post(&mutex);
		sem_post(&full);
		sem_post(&care);// dental is available for the next Customer
		sem_post(&confirm); //approval the payment
		sleep(1);
}
}
