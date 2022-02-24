//////////////////////////////////////////////////////////////////////////////////////////////
// The program should receive data from the ipc_sendfile program and write it in a file		//
//------------------------------------------------------------------------------------------//
// The different methods accepted will be:													//
//		* share memory																		//
//------------------------------------------------------------------------------------------//
// For the moment theses methods are implemented:											//
// 		* message passing 																	//
//		* message queue 																	//
// 		* pipe 																				//
// 		*																					//
//------------------------------------------------------------------------------------------//
// This file should take as argument:														//
//		* --help to print out a help text containing short description of all supported		//
//			command line arguments															//
//		* --file <filename> file used to write data											//
//		* --<method> <element to connect to the sender> to give the method use and the 		//
//			way to recognize the sender.													//
//////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/neutrino.h>
#include <getopt.h>
#include <sys/dispatch.h>
#include <mqueue.h>
#include <fcntl.h>
#include <time.h>
#include "ipc_common_file.h"

char * filename;
int debug = DEBUG_VALUE; // variable to see each step
FILE* fptr;

void ipc_message(char * filename);
void ipc_queue(char * filename);
int writing(char * data, char * filename, unsigned data_size);
void ipc_pipe(char * filename);


int main (int argc, char *argv[])
{
	arguments argumentsProvided = analyseArguments(argc,argv);

	switch (argumentsProvided.protocol) //launching the correct function
	{
		case NONE:
			printf("Error. Missing arguments or wrong arguments. Use --help to know which arguments you can use\n");
			exit(EXIT_FAILURE);
		case HELP:
			break;
		case MSG:
			if (strlen(argumentsProvided.filename)==0)
			{
				printf("Filename must be specified. Abort\n");
				return EXIT_FAILURE;
			}
			ipc_message(argumentsProvided.filename);
			break;
		case QUEUE:
			if (strlen(argumentsProvided.filename)==0)
			{
				printf("Filename must be specified. Abort\n");
				return EXIT_FAILURE;
			}
			ipc_queue(argumentsProvided.filename);
			break;
		case PIPE:
			if (strlen(argumentsProvided.filename)==0)
			{
				printf("Filename must be specified. Abort\n");
				return EXIT_FAILURE;
			}
			ipc_pipe(argumentsProvided.filename);
			break;
		default:
			break;
	}

	return EXIT_SUCCESS;

}

int writing(char * data, char * filename,unsigned data_size)
{
	if (debug) printf("Try writing the file %s with %u bytes\n", filename, data_size);

	//write the text
	fwrite(data, data_size, 1, fptr);

	return EXIT_SUCCESS;
}

void ipc_message(char * filename)
{
	send_by_msg msg;
	int rcvid;
	char* data;
	int status;
	name_attach_t *attach;


	attach = name_attach(NULL, INTERFACE_NAME,0);
	if (attach == NULL)
	{
		perror("name_attach");
		exit(EXIT_FAILURE);
	}


	fptr = fopen64(filename, "wb");  //Create/open the file in write binary mode
	if (fptr==NULL)
	{
		perror("Openfile");
		exit(EXIT_FAILURE);
	}

	//Receive mode until all data are received

	while(1)
	{
		if (debug) printf("Waiting for message\n");

		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);
		if (rcvid == -1)
		{ //was there an error receiving msg?
			printf("error number: %d\n", errno);
			perror("MsgReceive"); //look up errno code and print
			exit(EXIT_FAILURE); //give up
		}
		else if (rcvid ==0)
		{
			if (msg.pulse.code == _PULSE_CODE_DISCONNECT)
			{
				printf("Client disconnected, the copy is finished.\n");
				if (ConnectDetach(msg.pulse.scoid) == -1)
				{
					perror("ConnectDetach");
				}
				break;
			}
			else
			{
				printf("unknown pulse received, code = %d\n", msg.pulse.code);
				exit(EXIT_FAILURE);
			}

		}
		else if (rcvid > 0)
		{
			if (msg.msg.msg_type != CPY_IOV_MSG_TYPE) // we expect only CPY_IOV_MSG_TYPE
			{
				printf("receive message type %d, expected: %d\n", msg.msg.msg_type, CPY_IOV_MSG_TYPE);
				perror("MsgReceive, unknown type.");
				exit(EXIT_FAILURE);
			}

			data = calloc(msg.msg.data_size,sizeof(char));
			if (data == NULL)
			{
				perror("MsgError");
				free(data);
				exit(EXIT_FAILURE);
			}

			if (debug) printf("allocating %lu bytes\n", msg.msg.data_size);

			//Receive data and call writing function
			status = MsgRead(rcvid, data, msg.msg.data_size, sizeof(iov_msg));
			if (status == -1)
			{
				perror("MsgRead");
				if (debug) printf("Length of MsgRead expected : %lu\n", msg.msg.data_size);
				free(data);
				exit(EXIT_FAILURE);
			}

			if (debug) printf("%lu bytes to write\n", strlen(data));
			writing(data, filename, msg.msg.data_size);
			free(data);
			status = MsgReply(rcvid, EOK, NULL, 0);
			if (status == -1)
			{
				perror("MsgReply");
				exit(EXIT_FAILURE);
			}

		}
	}


	//close the file
	status = fclose(fptr);
	if (status !=0)
	{
		perror("fclose error");
		exit(EXIT_FAILURE);
	}
	name_detach(attach, 0);
	return;
}



void ipc_queue(char * filename)
{
	mqd_t queue;
	struct mq_attr queueAttr; //variable for the attributes of the queue
	char* data;
	ssize_t bytes_received;
	int ret;
	struct timespec abs_timeout; //give a timer while waiting for queue messages
	unsigned int prio = QUEUE_PRIORITY;

	// Giving attributes
	queueAttr.mq_maxmsg = MAX_QUEUE_MSG;
	queueAttr.mq_msgsize = MAX_QUEUE_MSG_SIZE;

	// Opening the queue
	queue = mq_open(INTERFACE_NAME, O_CREAT | O_RDONLY , S_IRWXU | S_IRWXG, &queueAttr);
	if (queue == -1) {
	     perror ("mq_open()");
	     exit(EXIT_FAILURE);
	}
	printf ("Successfully opened my_queue:\n");

	fptr = fopen64(filename, "wb");  //Create/open the file in write binary mode
	if (fptr==NULL)
	{
		perror("Openfile");
		exit(EXIT_FAILURE);
	}

	ret = mq_getattr (queue, &queueAttr);
	if (ret == -1) {
		perror ("mq_getattr()");
		exit(EXIT_FAILURE);
	}
	if (debug) printf("Messages: %ld; send waits: %ld; receive waits: %ld\n\n", queueAttr.mq_curmsgs, queueAttr.mq_sendwait, queueAttr.mq_recvwait);

	data = malloc(MAX_QUEUE_MSG_SIZE);
	if (data == NULL)
	{
		perror("mallocError");
		exit(EXIT_FAILURE);
	}

	clock_gettime(CLOCK_REALTIME, &abs_timeout);
	abs_timeout.tv_sec += 30;

	//receiving message from queue ... don't wait more than 30s
	bytes_received = mq_timedreceive (queue, data, MAX_QUEUE_MSG_SIZE, &prio,&abs_timeout);
	if (bytes_received == -1)
	{
		 if (errno == ETIMEDOUT) {
			printf ("No queue message for 30s. Abort\n");
			free(data);
			exit(EXIT_SUCCESS);
		 }
		 else
		 {
			perror ("mq_timedreceive()");
			exit(EXIT_FAILURE);
		 }
	}
	else {
		if (debug) printf("Receiving %lu bytes\n", bytes_received);
	  }

	//writing on the file
	writing(data, filename, bytes_received);
	while(1)
	{
		clock_gettime(CLOCK_REALTIME, &abs_timeout);
		abs_timeout.tv_sec +=1;
		if (debug) printf("Messages: %ld; send waits: %ld; receive waits: %ld\n\n", queueAttr.mq_curmsgs, queueAttr.mq_sendwait, queueAttr.mq_recvwait);
		//receiving message from queue ... don't wait more than 30s
		bytes_received = mq_timedreceive (queue, data, MAX_QUEUE_MSG_SIZE, &prio,&abs_timeout);
		if (bytes_received == -1)
		{
		     if (errno == ETIMEDOUT) {
		        printf ("No more queue message. It should be a success.\n");
		        break;
		     }
		     else
		     {
		        perror ("mq_timedreceive()");
		        free(data);
		        exit(EXIT_FAILURE);
		     }
		}
		else {
			if (debug) printf("Receiving %lu bytes\n", bytes_received);
		  }

		//writing on the file
		writing(data, filename, bytes_received);

	}
	free(data);

	//close the file
	ret = fclose(fptr);
	if (ret !=0)
	{
		perror("fclose error");
		exit(EXIT_FAILURE);
	}

	/* Unlink and then close the message queue. */
	ret = mq_unlink (INTERFACE_NAME);
	if (ret == -1) {
	 perror ("mq_unlink()");
	 exit(EXIT_FAILURE);
	}

	ret = mq_close(queue);
	if (ret == -1)
	{
	 perror ("mq_close()");
	 exit(EXIT_FAILURE);
	}
}

void ipc_pipe(char * filename)
{
	int status;
	int fd; //pipe file descriptor
	int size_read = 0; //size of data read on the pipe
	char * data;

	fptr = fopen64(filename, "wb");  //Create/open the file in write binary mode
	if (fptr==NULL)
	{
		perror("Openfile");
		exit(EXIT_FAILURE);
	}

	status = mkfifo(INTERFACE_NAME, S_IRWXU | S_IRWXG);
	if (status == -1 && errno != EEXIST)
	{
		perror("mkfifo");
		exit(EXIT_FAILURE);
	}

	fd = open(INTERFACE_NAME,O_RDONLY);

	data = malloc(PIPE_BUFF);
	while (size_read == 0 ) //waiting for data on the pipe
	{
		sleep(1);
		size_read = read(fd, data, PIPE_BUFF);
	}
	writing(data, filename, size_read);
	if (debug) printf("%d bytes written on the file\n", size_read);


	while(size_read > 0)
	{
		size_read = read(fd, data, PIPE_BUFF);
		writing(data, filename, size_read);
		if (debug) printf("%d bytes written on the file\n", size_read);
	}

	free(data);
	//Closing pipe and file
	status = close(fd);
	if (status != 0)
	{
		perror("Pipe close");
	}

	printf("Finished writing data.\n");

	status = fclose(fptr);
	if (status == -1)
	{
	 perror ("file_close()");
	}

	status = remove(INTERFACE_NAME);
	if (status != 0)
	{
		perror("pipe remove");
	}

}

/*
 * ipc_receivefile.c
 *
 *  Created on: Feb 15, 2022
 *      Author: romain
 */


