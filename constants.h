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
#define UPDATE_PRIO_MSG_TYPE (_IO_MAX+201)
#define GET_CAR_INFO_MSG_TYPE (_IO_MAX+202)

enum dirs {
	North = 0,
	East,
	South,
	West
};

typedef struct {
	int row;
	int col;
} coordinates_t;

typedef struct update_msg {
	uint16_t type;
	int state;
} update_prio_msg_t;

typedef struct get_prio_msg {
	uint16_t type;
	coordinates_t coordinates;
	int state;
} get_prio_msg_t;

typedef struct get_prio_resp {
	unsigned priority;
} get_prio_resp_t;

typedef struct car{
	int id;
	enum dirs direction;
	int in_grid; //1=yes, 0=no
	coordinates_t coordinates;
} car_t;

typedef struct {
	uint16_t type;
	car_t car;
} car_info_msg_t;

typedef struct{
	int signal; //1=red, 0=green
} car_info_resp_t;


#endif /* CONSTANTS_H_ */
