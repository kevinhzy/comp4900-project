/*
 * constants.h
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <stdint.h>
#include <sys/iomsg.h>
#include <sys/mman.h>

#define CTRL_SERVER_NAME "Block Controller"

#define GET_PRIO_MSG_TYPE (_IO_MAX+200)
#define TRAFFIC_COUNT_MSG_TYPE (_IO_MAX+201)


typedef struct traffic_count_msg {
	uint16_t type;
	unsigned count;
	int x;
	int y;
} traffic_count_msg_t;

typedef struct get_prio_msg {
	uint16_t type;
} get_prio_msg_t;

typedef struct get_prio_resp {
	unsigned priority;
} get_prio_resp_t;

typedef struct{
	int direction;
} car_dir_t;

extern int INTERSECTIONS;
extern int WIDTH_SIZE;
#endif /* CONSTANTS_H_ */
