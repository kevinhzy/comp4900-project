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

	int server_coid;
	INTERSECTIONS = atoi(argv[1]);
	WIDTH_SIZE = atoi(argv[2]);
	pid_t pid;

	if(fork() == 0){

		intersection_info_t intersection_info = {.type = GET_INTERSECTIONS_INFO, .num_of_intersections = INTERSECTIONS};
		if ((server_coid = name_open(CTRL_SERVER_NAME, 0)) == -1){
			printf("Simulator could not establish connection to the BlockController\n");
	        return EXIT_FAILURE;
	    }
	    if((MsgSend(server_coid, &intersection_info, sizeof(intersection_info), NULL, sizeof(NULL)))==-1){ //NULL
	    	printf("intersection_msg send error\n");
			exit(EXIT_FAILURE);
	    }
	    name_close(server_coid);
	}else{

		// Allow this process to spawn child processes.
		procmgr_ability(0, PROCMGR_AID_SPAWN);

		// Inheritance flag has to equal 0 for children to spawn properly.
		struct inheritance inherit;
		inherit.flags = 0;

		int row = 0;
		int column = 0;
		char row_buff[50], column_buff[50];

		for (int i=0; i<INTERSECTIONS; i++){
			row = i / WIDTH_SIZE;
			column = i % WIDTH_SIZE;

			char *args[] = {"sample_Intersection",itoa(row, row_buff, 10),itoa(column, column_buff, 10), NULL};
			if((pid = spawn("/tmp/sample_Intersection", 0, NULL, &inherit, args, environ))==-1){
				printf("[Sim] Failed to spawn intersection assigned location (%d, %d) | PID: %d\n",row,column, pid);
			}else{
				printf("[Sim] Succesfully spawned intersection with location (%d, %d)| PID: %d\n", row, column, pid);
			}
		}
		return 0;
	}
}

