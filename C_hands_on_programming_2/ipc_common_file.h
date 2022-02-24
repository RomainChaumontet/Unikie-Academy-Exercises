/*
 * ipc_common_file.h
 *
 *  Created on: Feb 24, 2022
 *      Author: romain
 */

#ifndef IPC_COMMON_FILE_H_
#define IPC_COMMON_FILE_H_


#include <sys/iomsg.h>

#define INTERFACE_NAME "ipcCopyFile"
#define DEBUG_VALUE 0
#define MAX_MSG_BUFF_SIZE 1000000
#define MAX_QUEUE_MSG_SIZE 409600
#define MAX_QUEUE_MSG 1024
#define CPY_IOV_MSG_TYPE (_IO_MAX + 5)
#define QUEUE_PRIORITY 5
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
	protocol_t protocol;
	char * filename;
} arguments;

typedef union
{
	uint16_t msg_type;
	struct _pulse pulse;
	iov_msg msg;
} send_by_msg;

arguments analyseArguments(int argc, char *argv[]);

#endif /* IPC_COMMON_FILE_H_ */
