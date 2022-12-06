#include <stdio.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>

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

// Function pointer for auto process terminator thread.
void *auto_terminator(void *);

int main(int argc, char *argv[]){

	// Create an auto terminator thread.
	if((pthread_create(NULL, NULL, auto_terminator, NULL)) != 0) {
		printf("Error creating auto terminator thread\n");
		exit(EXIT_FAILURE);
	}

    int run_duration = 12000;
    pthread_t thdID0, thdID1, thdID2;
    coordinates_t args = {.row = atoi(argv[1]), .col = atoi(argv[2])};

    // Create the grabber thread.
    if((pthread_create(&thdID0, NULL, grabber, (void *)&args)) != 0){
    	printf("Error in grabber thread\n");
    	exit(EXIT_FAILURE);
    };
    // Set its priority to 2 so that it's higher than the direction threads.
    pthread_setschedprio(thdID0, 2);

    // Create the east_west thread.
    if((pthread_create(&thdID1, NULL, east_west, NULL)) != 0){
    	printf("Error in east-west thread\n");
    	exit(EXIT_FAILURE);
    };
    // Set its priority to 1 so that it's lower than the grabber thread.
    pthread_setschedprio(thdID1, 1);

    // Create the north_south thread.
    if((pthread_create(&thdID2, NULL, north_south, NULL)) != 0){
    	printf("Error in north-south thread\n");
    	exit(EXIT_FAILURE);
    };
    // Set its priority to 1 so that it's lower than the grabber thread.
    pthread_setschedprio(thdID2, 1);

    // Let program run for run_duration seconds.
    sleep(run_duration);
    return 0;
}

int get_duration(int priority)
{
	if(priority < 1){
		printf("[In] get_duration priority is 0 or -'ve\n");
		return 0;
	}else{
		printf("[In] Priority received is: %d\n", priority);
	}
	return (20 / priority);
}

void *east_west(void *arg)
{
    while(1)
    {
    	int ret_code;
    	ret_code = pthread_mutex_lock(&mutex);

    	duration = get_duration(priority);
    	if(ret_code == EOK){
            printf("[In] East to west is now green for %d seconds!\n", duration);
            sleep(duration);
            ret_code = pthread_mutex_unlock(&mutex);
            if(ret_code != EOK){
                printf("pthread_mutex_unlock() failed %s\n", strerror(ret_code));
            }
    	}
    	else{
    		printf("pthread_mutex_unlock() failed %s\n", strerror(ret_code));
    	}
    }
}

void *north_south(void *arg)
{
    while(1)
    {
    	int ret_code;
        ret_code = pthread_mutex_lock(&mutex);

        duration = get_duration(priority);
        if(ret_code == EOK){
            printf("[In] North to south is now green for %d seconds!\n", duration);
            sleep(duration);
            ret_code = pthread_mutex_unlock(&mutex);
            if(ret_code != EOK){
            	printf("pthread_mutex_unlock() failed %s\n", strerror(ret_code));
            }
        }else{
        	printf("pthread_mutex_unlock() failed %s\n", strerror(ret_code));
        }
    }
}

void *grabber(void *arg){

	int ret_code, coid;
	ret_code = pthread_mutex_lock(&mutex);
	coordinates_t *args = (coordinates_t *) arg;

	if(ret_code == EOK){
		// Acquire connection id from the server's name.
		coid = name_open(CTRL_SERVER_NAME, 0);

		get_prio_msg_t prio_msg = {.type = GET_PRIO_MSG_TYPE, .coordinates.row = args->row, .coordinates.col= args->col};
		traffic_count_msg_t traffic_msg = {.type = TRAFFIC_COUNT_MSG_TYPE, .count = rand() % (15 - 10 + 1) + 10};
		get_prio_resp_t prio_resp;

		// Send request to block controller asking for a priority for this intersection.
		MsgSend(coid, &prio_msg, sizeof(prio_msg), &prio_resp, sizeof(prio_resp));

		// Set the priority to the value obtained from the block controller.
		//printf("Received priority %u from server\n", prio_resp.priority);
		priority = prio_resp.priority;
		ret_code = pthread_mutex_unlock(&mutex);

        if(ret_code != EOK){
        	printf("pthread_mutex_unlock() failed %s\n", strerror(ret_code));
        }
		while(1){

			int ret_code;
			ret_code = pthread_mutex_lock(&mutex);
			if(ret_code == EOK){
				// Obtain traffic count info somehow...
				// Send traffic count to block controller and receive back a priority.
				MsgSend(coid, &traffic_msg, sizeof(traffic_msg), &prio_resp, sizeof(prio_resp));

				// Set the priority to the value obtained from the block controller.
				priority = prio_resp.priority;
				ret_code = pthread_mutex_unlock(&mutex);
				if(ret_code != EOK){
					printf("pthread_mutex_unlock() failed %s\n", strerror(ret_code));
				}
			}
			else{
				printf("pthread_mutex_unlock() failed %s\n", strerror(ret_code));
			}
		}
	}else{
		printf("pthread_mutex_unlock() failed %s\n", strerror(ret_code));
	}
}

void *auto_terminator(void* arg) {
	printf("Auto-terminator online. I shall guarantee this process slain\n");
	// Continuously poll every 1 second to see if
	// simulator (our parent process) has been terminated.
	while(1)
	{
		// The parent's PID being 1 indicates that this process
		// is now a child of init, indicating that its original
		// parent, simulator, has died.
		if (getppid() == 1) {
			// Kill this process so it doesn't linger and cause segmentation
			// fault issues for the next block controller that is run.
			kill(getpid(), SIGKILL);
		}
		sleep(1);
	}
}
