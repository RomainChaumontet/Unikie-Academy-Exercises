/*
 * copyfile.h
 *
 *  Created on: Feb 15, 2022
 *      Author: romain
 */

#ifndef COPYFILE_H_
#define COPYFILE_H_

#include <sys/iomsg.h>

#define MAXFILEMAME 4096
#define MAXSERVERNAME 4096
#define CPY_IOV_MSG_TYPE (_IO_MAX + 2)


typedef struct
{
	uint16_t msg_type;
	unsigned data_size;
} iov_msg;

#endif /* COPYFILE_H_ */
