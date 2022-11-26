#include <stdio.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>

#include "constants.h"

/*
 * Global variables for determining light alternation timing.
*/
volatile int priority;
volatile int duration;

/*
 * Mutex used to determine which direction has right of way.
*/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Thread function pointers.
*/
void *east_west(void *);
void *north_south(void *);
void *grabber(void *);

int main(int argc, char *argv[]){

    int run_duration = 120;
    pthread_t threadID;

    printf("Running sample intersection for %d seconds\n", run_duration);

    // Create the grabber thread.
    pthread_create(&threadID, NULL, grabber, NULL);
    // Set its priority to 2 so that it's higher than the direction threads.
    pthread_setschedprio(threadID, 2);

    // Create the east_west thread.
    pthread_create(&threadID, NULL, east_west, NULL);
    // Set its priority to 1 so that it's lower than the grabber thread.
    pthread_setschedprio(threadID, 1);

    // Create the north_south thread.
    pthread_create(&threadID, NULL, north_south, NULL);
    // Set its priority to 1 so that it's lower than the grabber thread.
    pthread_setschedprio(threadID, 1);


    printf("Threads have all been generated\n");

    // Let program run for run_duration seconds.
    sleep(run_duration);

    return 0;
}

int get_duration(int priority)
{
	printf("Priority received is: %d\n", priority);
	return 20 / priority;
}

void *east_west(void *arg)
{
    while(1)
    {
        pthread_mutex_lock(&mutex);
        duration = get_duration(priority);
        printf("East to west is now green for %d seconds!\n", duration);
        sleep(duration);
        pthread_mutex_unlock(&mutex);
    }
}

void *north_south(void *arg)
{
    while(1)
    {
        pthread_mutex_lock(&mutex);
        duration = get_duration(priority);
        printf("North to south is now green for %d seconds!\n", duration);
        sleep(duration);
        pthread_mutex_unlock(&mutex);
    }
}

void *grabber(void *arg)
{
	pthread_mutex_lock(&mutex);
	// Aquire connection id from the server's name.
	int coid = name_open(CTRL_SERVER_NAME, 0);

	get_prio_msg_t prio_msg = {.type = GET_PRIO_MSG_TYPE};
	traffic_count_msg_t traffic_msg = {.type = TRAFFIC_COUNT_MSG_TYPE, .count = 0};
	get_prio_resp_t prio_resp;
//	unsigned test_priority;

	// Send request to block controller asking for a priority.
	MsgSend(coid, &prio_msg, sizeof(prio_msg), &prio_resp, sizeof(prio_resp));

	// Set the priority to the value obtained from the block controller.
	printf("Received priority %u from server\n", prio_resp.priority);
	priority = prio_resp.priority;
	pthread_mutex_unlock(&mutex);

    while(1){
        pthread_mutex_lock(&mutex);
        // Obtain traffic count info somehow...
        // For now, set to arbitary value
        traffic_msg.count = 8;

        // Send traffic count to block controller and receive back a priority.
        MsgSend(coid, &traffic_msg, sizeof(traffic_msg), &prio_resp, sizeof(prio_resp));

        // Set the priority to the value obtained from the block controller.
        priority = prio_resp.priority;
        printf("Priority is now: %d\n", priority);
        pthread_mutex_unlock(&mutex);
    }
}
