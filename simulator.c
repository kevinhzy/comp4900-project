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

