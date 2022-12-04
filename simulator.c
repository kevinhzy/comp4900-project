#include <stdio.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <spawn.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/procmgr.h>
#include <sys/dispatch.h>
#include <string.h>

#include "constants.h"
int INTERSECTIONS;
int WIDTH_SIZE;

void *func_car(void *);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) {

	INTERSECTIONS = atoi(argv[1]);
	WIDTH_SIZE = atoi(argv[2]);
	int num_cars = 5; //rand() % (10 - 8 + 1) + 8; //randomly generate from 1 to 5 cars
	printf("TOTAL NUMBER OF CARS ON THE GRID: %d\n", num_cars);

	pid_t pid_;
	pid_t pid[INTERSECTIONS+1];
	car_t cars[num_cars];
	pthread_t threadID[num_cars];

	// Allow this process to spawn child processes.
	procmgr_ability(0, PROCMGR_AID_SPAWN);

	// Inheritance flag has to equal 0 for children to spawn properly.
	struct inheritance inherit;
	inherit.flags = 0;

	// ---------------------------
	// Spawn the block controller.
	// ---------------------------
	char inter_count_buff[12], width_buff[12];
	char *argsForCtrl[] = {"sample_BlockController", itoa(INTERSECTIONS, inter_count_buff, 10), itoa(WIDTH_SIZE, width_buff, 10), NULL};
	if((pid_ = spawn("/tmp/sample_BlockController", 0, NULL, &inherit, argsForCtrl, environ))==-1){
		printf("[Sim] Failed to spawn Block Controller.\n");
	}else{
		pid[0] = pid_;
		printf("[Sim] Succesfully spawned Block Controller\n");
	}

	// ------------------------
	// Spawn the intersections.
	// ------------------------
	int r_coordinate = 0, c_coordinate = 0;
	for(int i = 0; i<INTERSECTIONS; i++){

	    r_coordinate = i / WIDTH_SIZE;
	    c_coordinate = i % WIDTH_SIZE;

		char r_buff[50], c_buff[50];
		char *args[] = {"sample_Intersection",itoa(r_coordinate, r_buff, 10),itoa(c_coordinate, c_buff, 10), NULL};
		if((pid_ = spawn("/tmp/sample_Intersection", 0, NULL, &inherit, args, environ))==-1){
			printf("[Sim] Failed to spawn intersection assigned location (%d, %d) | PID: %d\n",r_coordinate,c_coordinate, pid_);
		}else{
			pid[i+1] = pid_;
			printf("[Sim] Succesfully spawned intersection with location (%d, %d) | PID: %d\n",r_coordinate,c_coordinate, pid_);
		}
	}

	srand(time(NULL));

	int sleep_time = 3;
	printf("[Sim] Sleep for %d seconds\n", sleep_time);
	sleep(sleep_time);

	printf("\n");
	fflush(stdout);

	int car_grid_allotment;

	for(int i = 0; i<num_cars; i++){

		// Populate information needed for each car.
		cars[i].id = i; //set car id
		cars[i].in_grid = 1; //each car is spawned inside the grid
		cars[i].direction = rand() % (3 - 0 + 1) + 0; //randomly generate 0, 1, 2, 3 for direction

		car_grid_allotment = rand() % INTERSECTIONS;
		cars[i].coordinates.row = car_grid_allotment / WIDTH_SIZE;
		cars[i].coordinates.col = car_grid_allotment % WIDTH_SIZE;
		printf("-- car assigned (%d, %d)\n", cars[i].coordinates.row, cars[i].coordinates.col);

		//create a thread for each car
//		car_t args = cars[i];
//		if((pthread_create(&threadID[i], NULL, func_car, (void *)&args)) != 0){
		if((pthread_create(&threadID[i], NULL, func_car, (void *)&cars[i])) != 0){
		    printf("[Sim] could not create car thread: %d\n", i);
		    exit(EXIT_FAILURE);
		};
	}

	//free resources of returned car threads
	for(int i = 0; i<num_cars; i++){
	pthread_join(threadID[i], NULL);
	}

	// kill blockController and intersections processes
//	for(int i = 0; i<INTERSECTIONS + 1; i++){
//		kill(pid[i], 9);
//	}

	printf("Main will sleep for a little...\n");
	sleep(30);
	printf("Main is done sleeping\n");

	return 0;
}

// Checks whether a car has fallen off the grid.
// Returns 1 if car will fall off, 0 otherwise.
int off_grid(coordinates_t coordinates, int new_dir){

	// The linear (1-dimensional) position of an intersection.
	int linear_position = (coordinates.row * WIDTH_SIZE) + (coordinates.col + 1);

	// Car is at top of grid and tries to move up.
	if (coordinates.row == 0 && new_dir == 0) {
		return 1;
	}
	// Car is at bottom of grid and tries to move down.
	if ((INTERSECTIONS - linear_position < WIDTH_SIZE) && new_dir == 3) {
		return 1;
	}
	// Car is at leftmost of grid and tries to move left.
	if (coordinates.col == 0 && new_dir == 1) {
		return 1;
	}
	// Car is at rightmost of grid and tries to move right.
	if ((coordinates.col == WIDTH_SIZE - 1 || linear_position == INTERSECTIONS) && new_dir == 2) {
		return 1;
	}

	return 0;
}

int opposite_direction_of[] = {3, 2, 1, 0};
int row_increment[] = {-1, 0, 0, 1};
int col_increment[] = {0, -1, 1, 0};

void *func_car(void *arg){

	int server_coid;
	car_t *car = (car_t *) arg;
	car_info_msg_t car_info_msg = {.type = GET_CAR_INFO_MSG_TYPE, .car = *car};
	car_info_resp_t car_info_resp;

	if((server_coid = name_open(CTRL_SERVER_NAME, 0)) == -1){
		pthread_mutex_lock(&mutex);
		printf("[Sim] car %d could not connect to the blockController\n", car->id);
		pthread_mutex_unlock(&mutex);
		exit(EXIT_FAILURE);
	}

	while(car->in_grid){

		pthread_mutex_lock(&mutex);
	//	printf("[Sim] car%d at (%d,%d) starts moving\n", car_info_msg.car.id, car_info_msg.car.coordinates.row, car_info_msg.car.coordinates.col);
		printf("[Sim] car%d at (%d,%d) requesting information from controller\n", car->id, car->coordinates.row, car->coordinates.col);
//		printf("[Sim] car%d at (%d,%d) requesting information from controller\n", car_info_msg.car.id, car_info_msg.car.coordinates.row, car_info_msg.car.coordinates.col);
		pthread_mutex_unlock(&mutex);

		if(MsgSend(server_coid, &car_info_msg, sizeof(car_info_msg), &car_info_resp, sizeof(car_info_resp)) == -1){
			pthread_mutex_lock(&mutex);
			printf("[Sim] car %d could not send info to the blockController\n", car->id);
			pthread_mutex_unlock(&mutex);
			exit(EXIT_FAILURE);
		};

		pthread_mutex_lock(&mutex);
		printf("[Sim] car%d finds (%d, %d)\n", car->id, car->coordinates.row, car->coordinates.col);
//		printf("[Sim] car%d finds (%d, %d)\n", car_info_msg.car.id, car_info_msg.car.coordinates.row, car_info_msg.car.coordinates.col);
		pthread_mutex_unlock(&mutex);

		//car's logic
		if(car_info_resp.signal==0){ //green light for car

			int old_dir = car->direction;
			int new_dir = rand() % 4;

			// Force the car to pick a direction that doesn't result in a U-turn.
			while(new_dir == opposite_direction_of[old_dir])
			{
				new_dir = rand() % 4;
			}

			pthread_mutex_lock(&mutex);
			printf("[Sim] car%d at (%d, %d) has old_dir: %d & new_dir: %d\n", car->id, car->coordinates.row, car->coordinates.col, old_dir, new_dir);
//			printf("[Sim] car%d at (%d, %d) has old_dir: %d & new_dir: %d\n", car_info_msg.car.id, car_info_msg.car.coordinates.row, car_info_msg.car.coordinates.col, old_dir, new_dir);
			pthread_mutex_unlock(&mutex);

			if (off_grid(car->coordinates, new_dir))
			{
				car->in_grid = 0;
				printf("[Sim] car %d fell off the grid\n", car->id);
			} else{
				int old_row = car->coordinates.row;
				int old_col = car->coordinates.col;

				car->coordinates.row += row_increment[new_dir];
				car->coordinates.col += col_increment[new_dir];

				pthread_mutex_lock(&mutex);
				printf("[Sim] car%d moved from (%d, %d) to (%d, %d)\n", car->id, old_row, old_col, car->coordinates.row, car->coordinates.col);
				pthread_mutex_unlock(&mutex);
			}
		}

		car_info_msg.car = *car;
	}

	if(name_close(server_coid) == -1){
		printf("[Sim] car %d could not close connection to the blockController\n", car->id);
		exit(EXIT_FAILURE);
	}
	//exit the thread if a car has fallen off the grid
	pthread_exit(NULL);
}
