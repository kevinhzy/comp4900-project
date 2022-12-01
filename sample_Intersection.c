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

typedef struct{
	int x;
	int y;
} arg_coordinates;

int main(int argc, char *argv[]){

    int run_duration = 120;
    pthread_t thdID0, thdID1, thdID2;

    printf("Running sample intersection for %d seconds\n", run_duration);

    arg_coordinates args;
    args.x = atoi(argv[1]);
    args.y = atoi(argv[2]);
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
    };    // Set its priority to 1 so that it's lower than the grabber thread.
    pthread_setschedprio(thdID1, 1);

    // Create the north_south thread.
    if((pthread_create(&thdID2, NULL, north_south, NULL)) != 0){
    	printf("Error in north-south thread\n");
    	exit(EXIT_FAILURE);
    };    // Set its priority to 1 so that it's lower than the grabber thread.
    pthread_setschedprio(thdID2, 1);
    printf("Threads have all been generated\n");

    // Let program run for run_duration seconds.
    sleep(run_duration);
    return 0;
}

int get_duration(int priority)
{
	if(priority <1){
		printf("get_duration priority is 0 or -'ve\n");
		exit(EXIT_FAILURE);
	}else{
		printf("Priority received is: %d\n", priority);
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
            printf("East to west is now green for %d seconds!\n", duration);
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
            printf("North to south is now green for %d seconds!\n", duration);
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
	arg_coordinates *args = (arg_coordinates *) arg;

	printf("%d, %d\n", args->x, args->y);

	ret_code = pthread_mutex_lock(&mutex);
	if(ret_code == EOK){
		// Acquire connection id from the server's name.
		coid = name_open(CTRL_SERVER_NAME, 0);

		get_prio_msg_t prio_msg = {.type = GET_PRIO_MSG_TYPE};
		traffic_count_msg_t traffic_msg = {.type = TRAFFIC_COUNT_MSG_TYPE, .count = 0, .x= args->x, .y = args->y};
		get_prio_resp_t prio_resp;
		//	unsigned test_priority;

		// Send request to block controller asking for a priority for this intersection.
		MsgSend(coid, &prio_msg, sizeof(prio_msg), &prio_resp, sizeof(prio_resp));

		// Set the priority to the value obtained from the block controller.
		printf("Received priority %u from server\n", prio_resp.priority);
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
				// For now, set to arbitary value
				traffic_msg.count = 8;

				// Send traffic count to block controller and receive back a priority.
				MsgSend(coid, &traffic_msg, sizeof(traffic_msg), &prio_resp, sizeof(prio_resp));

				// Set the priority to the value obtained from the block controller.
				priority = prio_resp.priority;
				printf("Priority is now: %d\n", priority);
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
