/*
 * copyfile.h
 *
 *  Created on: Feb 15, 2022
 *      Author: romain
 */

#ifndef COPYFILE_H_
#define COPYFILE_H_

#include <sys/iomsg.h>

#define MAXFILENAME 4096
#define MAXSERVERNAME 4096
#define MAXQUEUENAME 4096
#define MAX_QUEUE_MSG_SIZE 409600
#define MAX_QUEUE_MSG 1024
#define CPY_IOV_MSG_TYPE (_IO_MAX + 5)
#define QUEUE_PRIORITY 5
#define SHARED_MEMORY_NAME "shmCopyFile"

typedef struct
{
	uint16_t msg_type;
	unsigned long data_size;
} iov_msg;

typedef enum {
	NONE, MSG, QUEUE, PIPE, SHM, HELP
} protocol_t;

#endif /* COPYFILE_H_ */
