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

	char interBuff[50];
	char *argsForCtrl[] = {"sample_BlockController",itoa(INTERSECTIONS, interBuff, 10), NULL};
	if((pid_ = spawn("/tmp/sample_BlockController", 0, NULL, &inherit, argsForCtrl, environ))==-1){
		printf("[Sim] Failed to spawn Block Controller.\n");
	}else{
		pid[0] = pid_;
		printf("[Sim] Succesfully spawned Block Controller\n");
	}

	int x_coordinate = 0;
	int y_coordinate = 0;

	for(int i = 0; i<INTERSECTIONS; i++){

	    x_coordinate = i / WIDTH_SIZE;
	    y_coordinate = i % WIDTH_SIZE;


		char x_buff[50], y_buff[50];
		char *args[] = {"sample_Intersection",itoa(x_coordinate, x_buff, 10),itoa(y_coordinate, y_buff, 10), NULL};
		if((pid_ = spawn("/tmp/sample_Intersection", 0, NULL, &inherit, args, environ))==-1){
			printf("[Sim] Failed to spawn intersection assigned location (%d, %d) | PID: %d\n",x_coordinate,y_coordinate, pid_);
		}else{
			pid[i+1] = pid_;
			printf("[Sim] Succesfully spawned intersection with location (%d, %d)| PID: %d\n", x_coordinate, y_coordinate, pid_);
		}
	}

	printf("\n");

	fflush(stdout);
	for(int i = 0; i<num_cars; i++){
		cars[i].id = i; //set car id
		cars[i].in_grid = 1; //each car is spawned inside the grid
		cars[i].direction = rand() % (3 - 0 + 1) + 0; //randomly generate 0, 1, 2, 3 for direction
		if(INTERSECTIONS % WIDTH_SIZE == 0){ //if the grid is a rectangle
			cars[i].coordinates.x = rand() % ((INTERSECTIONS/WIDTH_SIZE - 1) - 0 + 1) + 0;
			cars[i].coordinates.y = rand() % ((WIDTH_SIZE - 1) - 0 + 1) + 0;
		}else{ //if the grid is not a rectangle
			cars[i].coordinates.x = rand() % (INTERSECTIONS/WIDTH_SIZE - 0 + 1) + 0;
			if(cars[i].coordinates.x == INTERSECTIONS/WIDTH_SIZE){
				cars[i].coordinates.y = rand() % ((INTERSECTIONS%WIDTH_SIZE - 1) - 0 + 1) + 0;
			}else{
				cars[i].coordinates.y =  rand() % ((WIDTH_SIZE - 1) - 0 + 1) + 0;
			}
		}
		//create a thread for each car
		car_t args = cars[i];
		if((pthread_create(&threadID[i], NULL, func_car, (void *)&args)) != 0){
		    printf("[Sim] could not create car thread: %d\n", i);
		    exit(EXIT_FAILURE);
		};
	}
	//free resources of returned car threads
	 for(int i = 0; i<num_cars; i++){
	    pthread_join(threadID[i], NULL);
	 }

	 //kill blockController and intersections processes
//	  for(int i = 0; i<INTERSECTIONS + 1; i++){
//	    kill(pid[i], 9);
//	   }
	return 0;
}
//checks whether a car has fallen off the grid
int off_grid(coordinates_t coordinates, int new_dir){
	if(INTERSECTIONS%WIDTH_SIZE == 0){
		if(coordinates.y == 0 && new_dir == 1){
			return 1;
		}else if(coordinates.y == (WIDTH_SIZE - 1) && new_dir == 2){
			return 1;
		}else if(coordinates.x == 0 && new_dir == 0){
			return 1;
		}else if(coordinates.x == (INTERSECTIONS/WIDTH_SIZE - 1) && new_dir == 3){
			return 1;
		}else{
			return 0;
		}
	}else{
		if(coordinates.y == 0 && new_dir == 1){
			return 1;
		}else if(coordinates.y == (WIDTH_SIZE - 1) && new_dir == 2){
			return 1;
		}else if(coordinates.x == 0 && new_dir == 0){
			return 1;
		}else if(coordinates.x == INTERSECTIONS/WIDTH_SIZE && new_dir == 3){
			return 1;
		}else if(coordinates.x == INTERSECTIONS/WIDTH_SIZE && coordinates.y == (INTERSECTIONS%WIDTH_SIZE - 1) && new_dir == 2){
			return 1;
		}else{
			return 0;
		}
	}
}

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
	pthread_mutex_lock(&mutex);
	printf("[Sim] car%d at (%d,%d) starts moving\n", car_info_msg.car.id, car_info_msg.car.coordinates.x, car_info_msg.car.coordinates.y);
	pthread_mutex_unlock(&mutex);
	while(car->in_grid){

		if(MsgSend(server_coid, &car_info_msg, sizeof(car_info_msg), &car_info_resp, sizeof(car_info_resp)) == -1){
			pthread_mutex_lock(&mutex);
			printf("[Sim] car %d could not send info to the blockController\n", car->id);
			pthread_mutex_unlock(&mutex);
			exit(EXIT_FAILURE);
		};

		pthread_mutex_lock(&mutex);
		printf("[Sim] car%d finds (%d, %d)\n", car_info_msg.car.id, car_info_msg.car.coordinates.x, car_info_msg.car.coordinates.y);
		pthread_mutex_unlock(&mutex);


		//car's logic
		if(car_info_resp.signal==0){ //green light for car

			int old_dir = car->direction;
			int new_dir;

			//car can not make u-turns
			if(old_dir == 0){
				while(new_dir == 3){ //if car is going up it can't go down
					new_dir = rand() % (3 - 0 + 1) + 0;
				}
			}else if(old_dir == 1){
				while(new_dir == 2){ //if car is going left it can't go right
					new_dir = rand() % (3 - 0 + 1) + 0;
				}
			}else if(old_dir == 2){
				while(new_dir == 1){ //if car is going right it can't go left
					new_dir = rand() % (3 - 0 + 1) + 0;
				}
			}else{
				while(new_dir == 0){ //if car is going down it can't go up
					new_dir = rand() % (3 - 0 + 1) + 0;
				}
			}

			pthread_mutex_lock(&mutex);
			printf("[Sim] car%d at (%d, %d) has old_dir: %d & new_dir: %d\n", car_info_msg.car.id, car_info_msg.car.coordinates.x, car_info_msg.car.coordinates.y, old_dir, new_dir);
			pthread_mutex_unlock(&mutex);

			if(old_dir == new_dir){
				if(new_dir == 0){
					if(off_grid(car->coordinates, new_dir) == 1){
						pthread_mutex_lock(&mutex);
						printf("[Sim] car %d fell off the grid\n", car->id);
						pthread_mutex_unlock(&mutex);

						car->in_grid = 0;
					}else{
						car->coordinates.x -= 1;
						pthread_mutex_lock(&mutex);
						printf("[Sim] car %d moved from (%d, %d) to (%d, %d)\n", car->id, car->coordinates.x + 1, car->coordinates.y, car->coordinates.x, car->coordinates.y);
						pthread_mutex_unlock(&mutex);

					}
				}else if(new_dir == 1){
					if(off_grid(car->coordinates, new_dir) == 1){
						pthread_mutex_lock(&mutex);
						printf("[Sim] car %d fell off the grid\n", car->id);
						pthread_mutex_unlock(&mutex);
						car->in_grid = 0;
					}else{
						car->coordinates.y -= 1;
						pthread_mutex_lock(&mutex);
						printf("[Sim] car %d moved from (%d, %d) to (%d, %d)\n", car->id, car->coordinates.x, car->coordinates.y + 1, car->coordinates.x, car->coordinates.y);
						pthread_mutex_unlock(&mutex);
					}
				}else if (new_dir == 2){
					if(off_grid(car->coordinates, new_dir) == 1){
						pthread_mutex_lock(&mutex);
						printf("[Sim] car %d fell of the grid\n", car->id);
						pthread_mutex_unlock(&mutex);
						car->in_grid = 0;
					}else{
						car->coordinates.y += 1;
						pthread_mutex_lock(&mutex);
						printf("[Sim] car %d moved from (%d, %d) to (%d, %d)\n", car->id, car->coordinates.x, car->coordinates.y - 1, car->coordinates.x, car->coordinates.y);
						pthread_mutex_unlock(&mutex);
					}
				}else{
					if(off_grid(car->coordinates, new_dir) == 1){
						pthread_mutex_lock(&mutex);
						printf("[Sim] car %d fell of the grid\n", car->id);
						pthread_mutex_unlock(&mutex);

						car->in_grid = 0;
					}else{
						car->coordinates.x += 1;
						pthread_mutex_lock(&mutex);
						printf("[Sim] car %d moved from (%d, %d) to (%d, %d)\n", car->id, car->coordinates.x, car->coordinates.y, car->coordinates.x + 1, car->coordinates.y);
						pthread_mutex_unlock(&mutex);

					}
				}
			}else if (old_dir == 0 && new_dir == 1){
				if(off_grid(car->coordinates, new_dir) == 1){
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d fell of the grid\n", car->id);
					pthread_mutex_unlock(&mutex);
					car->in_grid = 0;
				}else{
					car->coordinates.y -= 1;
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d moved from (%d, %d) to (%d, %d)\n", car->id, car->coordinates.x, car->coordinates.y + 1, car->coordinates.x, car->coordinates.y);
					pthread_mutex_unlock(&mutex);
				}
			}else if (old_dir == 0 && new_dir == 2){
				if(off_grid(car->coordinates, new_dir) == 1){
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d fell of the grid\n", car->id);
					pthread_mutex_unlock(&mutex);
					car->in_grid = 0;
				}else{
					car->coordinates.y += 1;
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d moved from (%d, %d) to (%d, %d)\n", car->id, car->coordinates.x, car->coordinates.y - 1, car->coordinates.x, car->coordinates.y);
					pthread_mutex_unlock(&mutex);
				}
			}else if (old_dir == 1 && new_dir == 0){
				if(off_grid(car->coordinates, new_dir) == 1){
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d fell of the grid\n", car->id);
					pthread_mutex_unlock(&mutex);
					car->in_grid = 0;
				}else{
					car->coordinates.x -= 1;
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d moved from (%d, %d) to (%d, %d)\n", car->id, car->coordinates.x + 1, car->coordinates.y, car->coordinates.x, car->coordinates.y);
					pthread_mutex_unlock(&mutex);
				}
			}else if (old_dir == 1 && new_dir == 3){
				if(off_grid(car->coordinates, new_dir) == 1){
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d fell of the grid\n", car->id);
					pthread_mutex_unlock(&mutex);
					car->in_grid = 0;
				}else{
					car->coordinates.x += 1;
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d moved from (%d, %d) to (%d, %d)\n", car->id, car->coordinates.x - 1, car->coordinates.y, car->coordinates.x, car->coordinates.y);
					pthread_mutex_unlock(&mutex);
				}
			}else if (old_dir == 2 && new_dir == 0){
				if(off_grid(car->coordinates, new_dir) == 1){
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d fell of the grid\n", car->id);
					pthread_mutex_unlock(&mutex);
					car->in_grid = 0;
				}else{
					car->coordinates.x -= 1;
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d moved from (%d, %d) to (%d, %d)\n", car->id, car->coordinates.x + 1, car->coordinates.y, car->coordinates.x , car->coordinates.y);
					pthread_mutex_unlock(&mutex);
				}
			}else if (old_dir == 2 && new_dir == 3){
				if(off_grid(car->coordinates, new_dir) == 1){
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d fell of the grid\n", car->id);
					pthread_mutex_unlock(&mutex);
					car->in_grid = 0;
				}else{
					car->coordinates.x += 1;
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d moved from (%d, %d) to (%d, %d)\n", car->id, car->coordinates.x - 1, car->coordinates.y, car->coordinates.x, car->coordinates.y);
					pthread_mutex_unlock(&mutex);
				}
			}else if (old_dir == 3 && new_dir == 1){
				if(off_grid(car->coordinates, new_dir) == 1){
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d fell of the grid\n", car->id);
					pthread_mutex_unlock(&mutex);
					car->in_grid = 0;
				}else{
					car->coordinates.y -= 1;
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d moved from (%d, %d) to (%d, %d)\n", car->id, car->coordinates.x, car->coordinates.y + 1, car->coordinates.x, car->coordinates.y);
					pthread_mutex_unlock(&mutex);
				}
			}else{
				if(off_grid(car->coordinates, new_dir) == 1){
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d fell of the grid\n", car->id);
					pthread_mutex_unlock(&mutex);
					car->in_grid = 0;
				}else{
					car->coordinates.y += 1;
					pthread_mutex_lock(&mutex);
					printf("[Sim] car %d moved from (%d, %d) to (%d, %d)\n", car->id, car->coordinates.x, car->coordinates.y - 1, car->coordinates.x, car->coordinates.y);
					pthread_mutex_unlock(&mutex);
				}
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
