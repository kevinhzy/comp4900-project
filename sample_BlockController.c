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

int INTERSECTIONS;

typedef struct{
	int x;
	int y;
} coordinates_t;

// WIP struct for server handling state of a specific intersection
typedef struct {
	pid_t pid;
	unsigned priority;
	unsigned traffic;
	coordinates_t coordinates;
} intersection_t;

// WIP generic message type
typedef union {
	uint16_t type;
	struct _pulse pulse;
	traffic_count_msg_t traffic_count;
} buffer_t;

// For finding array index with matching pid
// Use pid = -1 to find first available index for a newly connected intersection
int find (intersection_t* block, pid_t pid) {
	for (int i = 0; i < INTERSECTIONS; i++) {
		if (block[i].pid == pid) {
			return i;
		}
	}
	return -1;
}

int main(int argc, char **argv)
{
	buffer_t msg;
	int rcvid;
	intersection_t block[INTERSECTIONS-1];
	struct _msg_info info;
	get_prio_resp_t prio_resp;

	// Set up the block array with dummy pids, priority, traffic count and coordinates so they can be assigned later
	for (int i = 0; i < INTERSECTIONS; i++) {
		block[i].pid = -1;
		block[i].priority = 1;
		block[i].traffic = 0;
		block[i].coordinates.x = -1;
		block[i].coordinates.y = -1;
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
			    int idx = find(block, info.pid);
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
			printf("Message has been received by server\n");
			int idx;
			// we got a message, check its type and process the msg based on its type
			switch(msg.type)
			{
			case GET_PRIO_MSG_TYPE:
				// Initial assignment, default priority (1)
				// Save the PID to the block array
				idx = find(block, -1);
				block[idx].pid = info.pid;
				printf("Intersection %d assigned to %d\n", idx, info.pid);
				printf("Priority assigned: %d\n", block[idx].priority);
				prio_resp.priority = block[idx].priority;
				MsgReply(rcvid, 0, &(prio_resp), sizeof(prio_resp));
				break;
			case TRAFFIC_COUNT_MSG_TYPE:
				idx = find(block, info.pid);
				block[idx].traffic = msg.traffic_count.count;
				block[idx].coordinates.x = msg.traffic_count.x;
				block[idx].coordinates.y = msg.traffic_count.y;
				printf("Intersection %d updated traffic: %d\n", idx, block[idx].traffic);
				// TODO: Logic for changing priority based off traffic
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
				printf("Updated priority: %d\n", block[idx].priority);
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
