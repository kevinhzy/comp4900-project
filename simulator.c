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

int main(int argc, char **argv) {

	INTERSECTIONS = atoi(argv[1]);
	WIDTH_SIZE = atoi(argv[2]);
	pid_t pid;
	int status;

	// Allow this process to spawn child processes.
	procmgr_ability(0, PROCMGR_AID_SPAWN);

	// Inheritance flag has to equal 0 for children to spawn properly.
	struct inheritance inherit;
	inherit.flags = 0;

	char interBuff[50];
	char *argsForCtrl[] = {"sample_BlockController",itoa(INTERSECTIONS, interBuff, 10), NULL};
	if((pid = spawn("/tmp/sample_BlockController", 0, NULL, &inherit, argsForCtrl, environ))==-1){
		printf("[Sim] Failed to spawn Block Controller.\n");
	}else{
		printf("[Sim] Succesfully spawned Block Controller\n");
	}

	int x_coordinate = 0;
	int y_coordinate = 0;

	for (int i=0; i<INTERSECTIONS; i++){
		if(i == 0){
			x_coordinate = 0;
			y_coordinate = 0;
		}else if(i%WIDTH_SIZE == 0){
			x_coordinate += 1;
			y_coordinate = WIDTH_SIZE-1;
		}else{
			y_coordinate = (i%WIDTH_SIZE)-1;
		}

		char x_buff[50], y_buff[50];
		char *args[] = {"sample_Intersection",itoa(x_coordinate, x_buff, 10),itoa(y_coordinate, y_buff, 10), NULL};
		if((pid = spawn("/tmp/sample_Intersection", 0, NULL, &inherit, args, environ))==-1){
			printf("[Sim] Failed to spawn intersection assigned location (%d, %d) | PID: %d\n",x_coordinate,y_coordinate, pid);
		}else{
			printf("[Sim] Succesfully spawned intersection with location (%d, %d)| PID: %d\n", x_coordinate, y_coordinate, pid);
		}
	}

	while (1) {
		if ((pid = wait(&status)) == -1) {
			perror("wait() failed (no more child processes?)");
			exit(EXIT_FAILURE);
		}
		printf("a child terminated, pid = %d\n", pid);

		if (WIFEXITED(status)) {
			printf("child terminated normally, exit status = %d\n",
					WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
			printf("child terminated abnormally by signal = %X\n",
					WTERMSIG(status));
		} // else see documentation for wait() for more macros
	}

	return EXIT_SUCCESS;
}

