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

	srand(time(NULL));

	INTERSECTIONS = atoi(argv[1]);
	WIDTH_SIZE = atoi(argv[2]);
	int num_cars = rand() % (50 - (int)(INTERSECTIONS / 2)) + (int)(INTERSECTIONS / 2); //randomly generate INTERSECTIONS/2 to 50 cars
	printf("TOTAL NUMBER OF CARS ON THE GRID: %d\n", num_cars);

	pid_t pid_;
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

	// Currently, the cars fall off the map almost instantly, resulting
	// in the program finishing almost instantly and not allowing much time
	// to view the communications that occur between the intersections and
	// controller. This sleep allows for longer communications between the two.

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

enum dirs opposite_direction_of[] = {South, West, North, East};
int row_increment[] = {-1, 0, 0, 1};
int col_increment[] = {0, -1, 1, 0};

void *func_car(void *arg){

	int server_coid;
	car_t *car = (car_t *) arg;
	car_info_msg_t car_info_msg = {.type = GET_CAR_INFO_MSG_TYPE, .car = *car};
	car_info_resp_t car_info_resp;

	if((server_coid = name_open(CTRL_SERVER_NAME, 0)) == -1){
		printf("[Sim] car %d could not connect to the blockController\n", car->id);
		exit(EXIT_FAILURE);
	}

	while(car->in_grid){

		printf("[Sim] car%d at (%d,%d) requesting information from controller\n", car->id, car->coordinates.row, car->coordinates.col);

		if(MsgSend(server_coid, &car_info_msg, sizeof(car_info_msg), &car_info_resp, sizeof(car_info_resp)) == -1){
			printf("[Sim] car %d could not send info to the blockController\n", car->id);
			exit(EXIT_FAILURE);
		};
    
		printf("[Sim] car%d finds (%d, %d)\n", car->id, car->coordinates.row, car->coordinates.col);

		//car's logic
		if(car_info_resp.signal==0){ //green light for car

			int old_dir = car->direction;
			int new_dir = rand() % 4;

			// Force the car to pick a direction that doesn't result in a U-turn.
			while(new_dir == opposite_direction_of[old_dir]) {
				new_dir = rand() % 4;
			}

			printf("[Sim] car%d at (%d, %d) has old_dir: %d & new_dir: %d\n", car->id, car->coordinates.row, car->coordinates.col, old_dir, new_dir);

			if (off_grid(car->coordinates, new_dir))
			{
				car->in_grid = 0;
				printf("[Sim] car %d fell off the grid\n", car->id);
			} else{
				int old_row = car->coordinates.row;
				int old_col = car->coordinates.col;

				car->coordinates.row += row_increment[new_dir];
				car->coordinates.col += col_increment[new_dir];

				printf("[Sim] car%d moved from (%d, %d) to (%d, %d)\n", car->id, old_row, old_col, car->coordinates.row, car->coordinates.col);
			}
		}
		else {
			printf("[Sim] car%d waiting at red light\n", car->id);
			sleep(1);
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
