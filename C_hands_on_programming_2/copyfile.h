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
#define SHARE_MEMORY_BUFF 409600
#define PIPE_NAME "ipcPipeName"
#define PIPE_BUFF 4096

typedef struct
{
	uint16_t msg_type;
	unsigned long data_size;
} iov_msg;

typedef enum {
	NONE, MSG, QUEUE, PIPE, SHM, HELP
} protocol_t;

typedef struct
{
	volatile unsigned init_flag;  // has the shared memory and control structures been initialized
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int data_version;  // for tracking updates, 64-bit count won't wrap during lifetime of a system
	int data_size;
	int bothConnected;
	char text[SHARE_MEMORY_BUFF];
} shmem_t;

#endif /* COPYFILE_H_ */
