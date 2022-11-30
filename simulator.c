#include <stdio.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <spawn.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/procmgr.h>

int main(int argc, char **argv) {

	// Allow this process to spawn child processes.
	procmgr_ability(0, PROCMGR_AID_SPAWN);

	// Inheritance flag has to equal 0 for children to spawn properly.
	struct inheritance inherit;
	inherit.flags = 0;

	// grid_size: total number of intersections
	// width: the grid's width
	const int grid_size = 4, width = 2;
	int row, col;

	char str_row[3], str_col[3];
	char *inter_args[4] = {"sample_Intersection", str_row, str_col, NULL};

	pid_t pid;

	// TODO: Spawn sample_BlockController process for intersections to connect to.
	// Should be relatively straight forward. 1 call to spawn() will suffice.

	for (int count = 0; count < grid_size; count ++)
	{
		row = count / width;
		col = count % width;

		// Convert row and col to strings and copy them into command-line arguments.
		sprintf(inter_args[1], "%d", row);
		sprintf(inter_args[2], "%d", col);

		pid = spawn("sample_Intersection", 0, NULL, &inherit, inter_args, environ);
		if (pid == -1) {
			printf("[Sim] Failed to spawn intersection assigned location (%d, %d)\n", row, col);
		}else{
			printf("[Sim] Succesfully spawned intersection with location (%d, %d) | PID: %d\n", row, col, pid);
		}
	}

	return 0;
}
