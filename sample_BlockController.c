#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <process.h>
#include <string.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <time.h>

#include "constants.h"

#define DEFAULT_BLOCK_SIZE 4

int INTERSECTIONS;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// WIP struct for server handling state of a specific intersection
typedef struct {
	pid_t pid;
	unsigned priority;
	unsigned traffic;
	int ns_sig;
	int ew_sig;
	coordinates_t coordinates;
} intersection_t;

// WIP generic message type
typedef union {
	uint16_t type;
	struct _pulse pulse;
	get_prio_msg_t init_data;
	update_prio_msg_t cur_state;
	car_info_msg_t car_info;
} buffer_t;

// For finding array index with matching pid
// Use pid = -1 to find first available index for a newly connected intersection
int find_block_pid(intersection_t* block, pid_t pid) {
	for (int i = 0; i < INTERSECTIONS; i++) {
		if (block[i].pid == pid) {
			return i;
		}
	}
	return -1;
}


int find_block_coord(intersection_t* block, coordinates_t coordinates){
	for(int i = 0; i<INTERSECTIONS; i++){
		if(block[i].coordinates.col == coordinates.col && block[i].coordinates.row == coordinates.row){
			return i;
		}
	}
	return -1;
}

// Function pointer for auto process terminator thread.
void *auto_terminator(void *);

int main(int argc, char **argv)
{
	// Create an auto terminator thread.
	if((pthread_create(NULL, NULL, auto_terminator, NULL)) != 0) {
		printf("Error creating auto terminator thread\n");
		exit(EXIT_FAILURE);
	}

	if (argc < 2) {
		INTERSECTIONS = DEFAULT_BLOCK_SIZE;
	}
	else {
		INTERSECTIONS = atoi(argv[1]);
	}

	buffer_t msg;
	int idx, rcvid;
	intersection_t block[INTERSECTIONS];
	struct _msg_info info;
	get_prio_resp_t prio_resp;
	coordinates_t car_coords;
	car_info_resp_t car_response;

	// Set up the block array with dummy pids, priority, traffic count and coordinates so they can be assigned later
	for (int i = 0; i < INTERSECTIONS; i++) {
		block[i].pid = -1;
		block[i].priority = 1;
		block[i].traffic = 0;
		block[i].ew_sig = 0;
		block[i].ns_sig = 1;
		block[i].coordinates.col = -1;
		block[i].coordinates.row = -1;
	}

	// set random seed for temporary priority assignment
	srand(time(NULL));

	// register our name for a channel
	name_attach_t* attach;
	attach = name_attach(NULL, CTRL_SERVER_NAME, 0);
	printf("Server running on namespace: %s\n", CTRL_SERVER_NAME);

	while (1) {
		// receive message
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), &info);
		if (0 == rcvid) {
			// received a pulse
			switch (msg.pulse.code) {
			case _PULSE_CODE_DISCONNECT:
				printf("Received disconnect pulse\n");
				idx = find_block_pid(block, info.pid);
				block[idx].pid = -1;
				if (-1 == ConnectDetach(msg.pulse.scoid)) {
					perror("ConnectDetach");
				}
				break;
			case _PULSE_CODE_UNBLOCK:
				printf("Received unblock pulse\n");
				if (-1 == MsgError(msg.pulse.value.sival_int, -1)) {
					perror("MsgError");
				}
				break;

			default:
				printf("unknown pulse received, code = %d\n", msg.pulse.code);
			}
		} else {
			//printf("Message has been received by server\n");
			// we got a message, check its type and process the msg based on its type
			switch(msg.type)
			{
			case GET_CAR_INFO_MSG_TYPE:
				pthread_mutex_lock(&mutex);
				printf("[BlockC] car%d looking for (%d, %d)...\n", msg.car_info.car.id, msg.car_info.car.coordinates.row, msg.car_info.car.coordinates.col);
				pthread_mutex_unlock(&mutex);
				car_coords = msg.car_info.car.coordinates;
				if((idx = find_block_coord(block, car_coords)) == -1){
					printf("[BlockC] BlockC cannot find the intersection with the provided coordinates\n");
					exit(EXIT_FAILURE);
				}
				block[idx].traffic++;
				if (msg.car_info.car.direction == North || msg.car_info.car.direction == South) {
					car_response.signal = block[idx].ns_sig;
				} else {
					car_response.signal = block[idx].ew_sig;
				}
				if (car_response.signal == 0) {
					block[idx].traffic = 0;
				}
				if(MsgReply(rcvid, 0, &car_response, sizeof(car_response)) == -1){
					printf("[BlockC] BlockC cannot send signal to the car thread\n");
					exit(EXIT_FAILURE);
				};
				break;

			case GET_PRIO_MSG_TYPE:
				// Initial assignment, default priority (1)
				// Save the PID to the block array
				idx = find_block_pid(block, -1);
				block[idx].pid = info.pid;
				block[idx].coordinates.col = msg.init_data.coordinates.col;
				block[idx].coordinates.row = msg.init_data.coordinates.row;
				block[idx].ew_sig = msg.init_data.state;
				block[idx].ns_sig = (msg.init_data.state + 1) % 2;
				//printf("Intersection %d assigned to %d\n", idx, info.pid);
				//printf("Priority assigned: %d\n", block[idx].priority);
				prio_resp.priority = block[idx].priority;
				MsgReply(rcvid, 0, &(prio_resp), sizeof(prio_resp));
				break;

			case UPDATE_PRIO_MSG_TYPE:
				idx = find_block_pid(block, info.pid);
				block[idx].ew_sig = msg.cur_state.state;
				block[idx].ns_sig = (msg.init_data.state + 1) % 2;
				//printf("Intersection %d updated traffic: %d\n", idx, block[idx].traffic);
				// Rudimentary scaling for traffic priorities
				if (block[idx].traffic > 20) {
					block[idx].priority = 5;
				} else if (block[idx].traffic > 15){
					block[idx].priority = 4;
				} else if (block[idx].traffic > 10){
					block[idx].priority = 3;
				} else if (block[idx].traffic > 5){
					block[idx].priority = 2;
				} else{
					block[idx].priority = 1;
				}
				prio_resp.priority = block[idx].priority;
				MsgReply(rcvid, 0, &(prio_resp), sizeof(prio_resp));
				break;
			default:
				perror("MsgError");
			}
		}
	}
	return 0;
}

void *auto_terminator(void* arg) {
	printf("Auto-terminator online. I shall guarantee this process slain\n");
	// Continuously poll every 1 second to see if
	// simulator (our parent process) has been terminated.
	while(1)
	{
		// The parent's PID being 1 indicates that this process
		// is now a child of init, indicating that its original
		// parent, simulator, has died.
		if (getppid() == 1) {
			// Kill this process so it doesn't linger and cause segmentation
			// fault issues for the next block controller that is run.
			kill(getpid(), SIGKILL);
		}
		sleep(1);
	}
}
