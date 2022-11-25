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

#define CTRL_SERVER_NAME "Block Controller"

typedef struct {
	pid_t pid;
	unsigned priority;
} intersection_t;



int main(int argc, char **argv)
{
	msg_t msg;
	int rcvid;
	intersection_t block[4];
	struct _msg_info info;


	// register our name for a channel
	name_attach_t* attach;
	attach = name_attach(NULL, CTRL_SERVER_NAME, 0);

	while (1) {
		//receive message
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), &info);
		if (0 == rcvid) {
			//received a pulse
			switch (msg.pulse.code) {
			case _PULSE_CODE_DISCONNECT:
			    printf("Received disconnect pulse\n");
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
			// we got a message, check its type and process the msg based on its type
			switch(msg.type)
			{
			case GET_PRIO_MSG_TYPE:
				break;
			case TRAFFIC_COUNT_MSG_TYPE:
				break;
			default:
			    perror("MsgError");

			}
		}
	}
	return 0;
}
