//////////////////////////////////////////////////////////////////////////////////////////////
// The program should receive data from the ipc_sendfile program and write it in a file		//
//------------------------------------------------------------------------------------------//
// The different methods accepted will be:													//
//		* message queue 																	//
// 		* pipe 																				//
//		* share memory																		//
//------------------------------------------------------------------------------------------//
// For the moment theses methods are implemented:											//
// 		* message passing 																	//
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
#include "copyfile.h"

typedef union
{
	uint16_t msg_type;
	struct _pulse pulse;
	iov_msg msg;
} send_by_msg;



struct option long_options[] =
{
	  {"help",     no_argument, NULL, 'h'},
	  {"message",  required_argument, NULL, 'm'},
	  {"queue",  required_argument, NULL, 'q'},
	  {"pipe",  required_argument, NULL, 'p'},
	  {"shm",    required_argument, NULL, 's'},
	  {"file",  required_argument, NULL, 'f'},
	  {0, 0, 0, 0}
};

char filename[MAXFILENAME];
char servername[MAXSERVERNAME];
char queuename[MAXQUEUENAME];
int debug = 0; // variable to see each step
send_by_msg msg;
FILE* fptr;

void ipc_message(char filename[], char servername[]);
void ipc_queue(char filename[], char queuename[]);
int writing(char * data, char filename[], unsigned data_size);


int main (int argc, char *argv[])
{
	protocol_t protocol = NONE;
	int opt;
	while(1)
	{
		int option_index=0; //getopt_long stores the option index here
		opt = getopt_long (argc, argv, "hm:q:p:s:f:",long_options,&option_index);

		if (opt == -1) //no more options
			break;

		switch (opt)
		{
		case 'h':
			printf
			(
				"Ipc_sendfile is a program which get some data and write it to a file.\n"
				"The program accept the following arguments:\n"
				"	--help to print this information\n"
				"	--message <ServerName> to receive the data from the sender by IPC message passing\n"
				"	--queue <TBD> to receive the data from the sender by IPC queue #not yet implemented\n"
				"	--pipe <TBD> to receive the data from the sender by IPC pipe #not yet implemented\n"
				"	--shm <TBD> to receive the data from the sender with a shared memory #not yet implemented\n"
				" 	--file <filename> to specify the filename which has to be write\n"
			);
			protocol=HELP;
			break;
		case 'f':
			if (strlen(optarg) > MAXFILENAME)
			{
				printf("The name of the file is too long. Abort\n");
				break;
			}
			snprintf(filename, sizeof(filename),"%s",optarg);
			printf("The name of the file is %s\n",filename);
			break;
		case 'm':
			if (strlen(optarg) > MAXSERVERNAME)
			{
				printf("The servername is too long. Abort\n");
				exit(EXIT_FAILURE);
			}
			snprintf(servername, sizeof(servername),"%s",optarg);
			printf("The name of the server is %s\n",servername);
			protocol=MSG;
			break;
		case 'q':
			if (strlen(optarg) > MAXQUEUENAME)
			{
				printf("The name of the queu is too long. Abort\n");
				exit(EXIT_FAILURE);
			}
			snprintf(queuename, sizeof(queuename),"%s",optarg);
			printf("The name of the server is %s\n",queuename);
			protocol = QUEUE;
			break;
		case 'p':
		case 's':
			printf("This option is not implemented yet. Use --help to know witch ones are\n");
			exit(EXIT_FAILURE);
		case '?':
			break;
		default:
			break;
		}

	}

	switch (protocol) //launching the correct function
	{
		case NONE:
			printf("Error. Missing arguments or wrong arguments. Use --help to know which arguments you can use\n");
			exit(EXIT_FAILURE);
		case HELP:
			break;
		case MSG:
			if (strlen(filename)==0)
			{
				printf("Filename must be specified. Abort\n");
				return EXIT_FAILURE;
			}
			if (strlen(servername)==0)
			{
				printf("ServerName must be specified. Abort\n");
				return EXIT_FAILURE;
			}
			ipc_message(filename, servername);
			break;
		case QUEUE:
			if (strlen(filename)==0)
			{
				printf("Filename must be specified. Abort\n");
				return EXIT_FAILURE;
			}
			if (strlen(queuename)==0)
			{
				printf("The name of the queue must be specified. Abort\n");
				return EXIT_FAILURE;
			}
			ipc_queue(filename, queuename);
			break;
		default:
			break;
	}

	return EXIT_SUCCESS;

}

int writing(char * data, char filename[],unsigned data_size)
{
	if (debug) printf("Try writing the file %s with %u bytes\n", filename, data_size);
	int status;
	//write the text
	status = fwrite(data, data_size, 1, fptr);
	if (debug) printf( "Successfully wrote %d records\n", status );



	return EXIT_SUCCESS;

}

void ipc_message(char filename[], char servername[])
{
	if (debug) printf("Entering ipc_message\n");
	int rcvid;
	char* data;
	int status;
	name_attach_t *attach;


	attach = name_attach(NULL, servername,0);
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
				printf("Client disconnected, copy finished\n");
				if (ConnectDetach(msg.pulse.scoid) == -1)
				{
					perror("ConnectDetach");
				}
				break;
			}
			else
			{
				printf("unknown pulse received, code = %d\n", msg.pulse.code);
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
			int numberOfIov = msg.msg.data_size/4096+1;


			data = calloc(numberOfIov,4096);
			if (debug) printf("allocating %lu bytes\n", msg.msg.data_size);
			if (data == NULL)
			{
				if (MsgError(rcvid, ENOMEM ) == -1)
				{
					perror("MsgError");
					free(data);
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				//preparing iov
				iov_t siov[numberOfIov];
				for (int i=0; i<numberOfIov;i++)
				{
					SETIOV(&siov[i], &data[i], 4096);
				}

				//Receive data and call writing function
				status = MsgReadv(rcvid, siov, numberOfIov, sizeof(iov_msg));
				if (status == -1)
				{
					perror("MsgRead");
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



void ipc_queue(char filename[], char queuename[])
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
	queue = mq_open(queuename, O_CREAT | O_RDONLY , S_IRWXU | S_IRWXG, &queueAttr);
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

	while(1)
	{
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
		        printf ("No queue message for 30s. It should be a success.\n");
		        free(data);
		        break;
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
		free(data);

	}
	//close the file
	ret = fclose(fptr);
	if (ret !=0)
	{
		perror("fclose error");
		exit(EXIT_FAILURE);
	}

	/* Unlink and then close the message queue. */
	ret = mq_unlink (queuename);
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
unsigned int prio;

/*
 * ipc_receivefile.c
 *
 *  Created on: Feb 15, 2022
 *      Author: romain
 */


