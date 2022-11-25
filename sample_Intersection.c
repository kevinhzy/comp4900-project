#include <stdio.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>

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

    int run_duration = 60;
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


    // Let program run for run_duration seconds.
    sleep(run_duration);

    return 0;
}

void *east_west(void *arg)
{
    while(1)
    {
        pthread_mutex_lock(&mutex);
        duration = priority * 2;
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
        duration = priority * 2;
        printf("North to south is now green for %d seconds!\n", duration);
        sleep(duration);
        pthread_mutex_unlock(&mutex);
    }
}

void *grabber(void *arg)
{
    // Seeding random for example purposes.
    srand(time(NULL));

    while(1){
        pthread_mutex_lock(&mutex);
        // Request new priority from block controller via messaging.
        // For example purposes, assign it a random number between 1 and 4 (inclusive).
        priority = (rand() % 4) + 1;
        printf("Priority is now: %d\n", priority);
        pthread_mutex_unlock(&mutex);
    }
}
