#include "queue.h"
#include <stdio.h>

void launch_test(pthread_t threads[], int c, void *(*start_routine)(void*));
void *test1(void *vptr_args);
void *test2a(void *vptr_args);
void *test2b(void *vptr_args);
void *test3(void *vptr_args);
void *test4(void *vptr_args);

msg_queue_t queue;
int stop;

int main(int argc, char **argv)
{
	pthread_t threads[4];
	queue_create(&queue);

	printf(  "--- Test 1 ---\n");
	launch_test(threads, 2, &test1);

	printf("\n--- Test 2a ---\n");
	launch_test(threads, 4, &test2a);

	printf("\n--- Test 2b ---\n");
	launch_test(threads, 4, &test2b);

	printf("\n--- Test 3 ---\n");
	launch_test(threads, 1, &test3);

	printf("\n--- Test 4 ---\n");
	launch_test(threads, 2, &test4);

	queue_destroy(&queue);
	return 0;
}

void launch_test(pthread_t threads[], int threads_number, void *(*start_routine)(void*))
{
	int i;
	for(i=0; i<threads_number; i++)
		pthread_create(&threads[i], NULL, start_routine, (void*)i);
	for(i=0; i<threads_number; i++)
		pthread_join(threads[i], NULL);
}

// simple send & receive
void *test1(void *vptr_args)
{
	int thread_no = (int)vptr_args;
	char message[128];
	int buflen = 128, prio;
	switch(thread_no)
	{
		case 0: //sender
		strncpy(message, "To jest wiadomosc!", 128);
		queue_send(&queue, message, strlen(message) + 1, 1, 0, NULL);
		printf("Wyslano wiadomosc: %s\n", message);
		break;
		
		case 1: //receiver
		queue_recv(&queue, message, &buflen, &prio, 0, NULL);
		printf("Odebrano wiadomosc o dlugosci %d i priorytecie %d: %s\n", buflen, prio, message);
		break;
	}
}

// multiple writers
void *test2a(void *vptr_args)
{
	int thread_no = (int)vptr_args;
	char message[128];
	int buflen = 128, prio, i;
	time_t now;
	switch(thread_no)
	{
		case 0: //senders
		case 1:
		case 2:
		prio = 2 - thread_no;
		snprintf(message, 128, "Wiadomosc od watku %d", thread_no, prio);
		queue_send(&queue, message, strlen(message) + 1, prio, 0, NULL);
		break;
		
		case 3: //receiver
		// let's wait a ~second
		now = time(NULL);
		while(time(NULL) == now);
		for(i=0;i<3;i++)
		{
			queue_recv(&queue, message, &buflen, &prio, 0, NULL);
			printf("Odebrano wiadomosc o dlugosci %d i priorytecie %d: \"%s\"\n", buflen, prio, message);
		}
		break;
	}
}

// multiple writers/readers
void *test2b(void *vptr_args)
{
	int thread_no = (int)vptr_args;
	char message[128];
	int buflen = 128, prio, i;
	time_t now;
	switch(thread_no)
	{
		case 0: //senders
		case 1:
		case 2:
		queue_recv(&queue, message, &buflen, &prio, 0, NULL);
		printf("Watek %d otrzymal wiadomosc o priorytecie %d: \"%s\"\n", thread_no, prio, message);
		break;
		
		case 3: //receiver
		// let's wait a ~second
		strcpy(message, "Opozniona wiadomosc");
		buflen = strlen(message) + 1;
		for(i=0;i<3;i++)
		{
			now = time(NULL);
			while(time(NULL) == now);
			queue_send(&queue, message, buflen, i, 0, NULL);
		}
		break;
	}
}

// let's wait for 3 seconds
void *test3(void *vptr_args)
{
	int thread_no = (int)vptr_args;
	char message[128];
	int buflen = 128, prio;
	switch(thread_no)
	{
		case 0: //sender
		printf("Proba wyslania wiadomosci...\n");
		fflush(stdout);
		strncpy(message, "To jest wiadomosc!", 128);
		if(queue_send(&queue, message, strlen(message) + 1, 1, 3, NULL) == MSG_QUEUE_TIMEOUT)
			printf("Nastapil timeout.\n");
		break;
	}
}

// let's break sth
void *test4(void *vptr_args)
{
	int thread_no = (int)vptr_args;
	char message[128];
	int buflen = 128, prio;
	time_t now;
	switch(thread_no)
	{
		case 0: //sender
		stop = 0;
		printf("Proba wyslania wiadomosci...\n");
		fflush(stdout);
		strncpy(message, "To jest wiadomosc!", 128);
		if(queue_send(&queue, message, strlen(message) + 1, 1, 0, &stop) == MSG_QUEUE_STOPPED)
			printf("Nastapilo zatrzymanie przez zmienna stop.\n");
		break;

		case 1:
		now = time(NULL);
		while(time(NULL) == now);
		stop = 1;
	}
}
