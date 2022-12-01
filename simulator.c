#include <stdio.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <spawn.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/procmgr.h>

#include "constants.h"

int main(int argc, char **argv) {

	// Allow this process to spawn child processes.
	procmgr_ability(0, PROCMGR_AID_SPAWN);

	// Inheritance flag has to equal 0 for children to spawn properly.
	struct inheritance inherit;
	inherit.flags = 0;

	int INTERSECTIONS = atoi(argv[1]);
	int WIDTH_SIZE = atoi(argv[2]);

	int x_coordinate = 0;
	int y_coordinate = 0;

	pid_t pid;

	for (int i=1; i<=INTERSECTIONS; i++){
		if(i%WIDTH_SIZE == 0){
			x_coordinate += 1;
			y_coordinate = WIDTH_SIZE - 1;
		}else{
			y_coordinate = (i%WIDTH_SIZE)-1;
		}

		char x_buff[50], y_buff[50];
		char *args[] = {"sample_Intersection",itoa(x_coordinate, x_buff, 10),itoa(y_coordinate, y_buff, 10), NULL};
		if((pid = spawn("sample_Intersection", 0, NULL, &inherit, args, environ))==-1){
			printf("[Sim] Failed to spawn intersection assigned location (%d, %d)\n",x_coordinate,y_coordinate);
		}else{
			printf("[Sim] Succesfully spawned intersection with location (%d, %d)| PID: %d\n", x_coordinate, y_coordinate, pid);
		}
	}

	return 0;
}
