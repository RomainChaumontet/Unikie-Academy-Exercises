//////////////////////////////////////////////////////////////////////////////////////////////
// The program should read a given file and send it to ipc_receivefile using a method that 	//
// is given as a command-line argument. 													//
//------------------------------------------------------------------------------------------//
// The different methods accepted will be:													//
// 		* pipe 																				//
//		* share memory																		//
//------------------------------------------------------------------------------------------//
// For the moment theses methods are implemented:											//
//		* message queue 																	//
// 		* message passing																	//
//------------------------------------------------------------------------------------------//
// This file should take as argument:														//
//		* --help to print out a help text containing short description of all supported		//
//			command line arguments															//
//		* --file <filename> file used to read data											//
//		* --<method> <element to connect to the receiver> to give the method use and the 	//
//			way to recognize the receiver.													//
//////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/neutrino.h>
#include <getopt.h>
#include <sys/iofunc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/dispatch.h>
#include <mqueue.h>
#include <fcntl.h>
#include "copyfile.h"


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

int ipc_message(char filename[], char servername[]);
long int findSize(char file_name[]);
void ipc_queue(char filename[], char queuename[]);

char filename[MAXFILENAME];
char servername[MAXSERVERNAME];
char queuename[MAXQUEUENAME];
iov_msg msg;
char* data;
int debug =1;

int main (int argc, char *argv[])
{
	protocol_t protocol = NONE;
	int opt;
	while(1) //loop for taking care of arguments
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
				"Ipc_sendfile is a program which read a file and send the data to another program.\n"
				"The program accept the following arguments:\n"
				"	--help to print this information\n"
				"	--message <ServerName> to send the data to the receiver by IPC message passing\n"
				"	--queue <Queue name> to send the data to the receiver by IPC queue\n"
				"	--pipe <TBD> to send the data to the receiver by IPC pipe #not yet implemented\n"
				"	--shm <TBD> to send the data to the receiver with a shared memory #not yet implemented\n"
				" 	--file <filename> to specify the filename which has to be read\n"
			);
			protocol=HELP;
			break;
		case 'f':
			if (strlen(optarg) > MAXFILENAME)
			{
				printf("The name of the file is too long");
				break;
			}
			snprintf(filename, sizeof(filename),"%s",optarg);
			printf("The name of the file is %s\n",filename);
			break;
		case 'm':
			if (strlen(optarg) > MAXSERVERNAME)
			{
				printf("The servername is too long. Abort\n");
				break;
			}
			snprintf(servername, sizeof(servername),"%s",optarg);
			printf("The name of the server is %s\n",servername);
			protocol = MSG;
			break;
		case 'q':
			if (strlen(optarg) > MAXQUEUENAME)
			{
				printf("The name of the queue is too long. Abort\n");
				exit(EXIT_FAILURE);
			}
			if (optarg[0] == '/')
			{
				snprintf(queuename, sizeof(queuename),"/ipc_queue%s",optarg);
			}
			else
			{
				snprintf(queuename, sizeof(queuename),"/ipc_queue/%s",optarg);
			}
			printf("The name of the queue is %s\n",queuename);
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

	switch (protocol) //launching correct function
		{
			case NONE:
				printf("Error. Missing arguments or wrong arguments. Use --help to know which arguments you can use\n");
				return EXIT_FAILURE;
				break;
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
					printf("Servername must be specified. Abort\n");
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

int ipc_message(char filename[], char servername[])
{
	long int file_size = findSize(filename);
	int coid = -1;
	int numberOfIov = file_size/4096+2; // +2 for the header and the last package.
	iov_t siov[numberOfIov];
	int fd;
	int size_read;
	int status;

	//locate the server
	while (coid == -1)
	{
		coid = name_open(servername,0);
		printf("Waiting for %s\n", servername);
		sleep(5);
	}

	msg.msg_type = CPY_IOV_MSG_TYPE;

	fd = open(filename, O_RDONLY | O_LARGEFILE, S_IRUSR | S_IWUSR );


	msg.data_size = file_size;
	SETIOV(&siov[0], &msg, sizeof(msg));

	char* buffer = calloc(numberOfIov-1,4096);

	for (int i=0; i < numberOfIov-1;i++) //filling buffer
	{
		size_read = read(fd, &buffer[i], 4096);
		// Test for error
		if( size_read == -1 )
		{
			perror( "Error reading myfile.dat" );
			free(buffer);
			exit(EXIT_FAILURE);
		}

		SETIOV(&siov[i+1], &buffer[i], size_read);
		if (debug) printf("adding one package of %d bytes\n",size_read);
	}
	printf("Send a msg with type: %d\n", msg.msg_type);
	status = MsgSendvs(coid, siov, numberOfIov, NULL, 0);
	if (status == -1)
	{ //was there an error sending to server?
		perror("MsgSend");
		free(buffer);
		exit(EXIT_FAILURE);
	}

	if (debug) printf("liberate the buffer\n");

	free(buffer);

	if (debug) printf("all data sent\n");
	close(fd);

	return EXIT_SUCCESS;
}



long int findSize(char file_name[])
{
	// opening the file in read mode
	FILE* fp = fopen(file_name, "r");

	// checking if the file exist or not
	if (fp == NULL) {
		printf("File Not Found!\n");
		exit(EXIT_FAILURE);
	}

	fseek(fp, 0L, SEEK_END);

	// calculating the size of the file
	long int res = ftell(fp);

	// closing the file
	fclose(fp);

	return res;
}


void ipc_queue(char filename[], char queuename[])
{
	long int file_size = findSize(filename);
	long int bytes_already_read = 0;
	mqd_t queue = -1;
	struct mq_attr queueAttr; //variable for the attributes of the queue
	char* data;
	int ret;
	int fd;
	int size_read;
	unsigned int prio = QUEUE_PRIORITY;

	// Giving attributes
	queueAttr.mq_maxmsg = MAX_QUEUE_MSG;
	queueAttr.mq_msgsize = MAX_QUEUE_MSG_SIZE;


	// Opening the queue
	queue = mq_open(queuename, O_WRONLY , S_IRWXU | S_IRWXG, &queueAttr);

	// If no queue -> the receiver is not launched yet... waiting for it
	while (queue == -1)
	{
		if (errno == ENOENT)
		{
			printf("Waiting for the receiver to connect to the queue named %s\n", queuename);
			queue = mq_open(queuename, O_WRONLY , S_IRWXU | S_IRWXG, &queueAttr);
			sleep(2);
		}
		else
		{
			perror("mq_open()");
			exit(EXIT_FAILURE);
		}

	}
	printf ("Successfully opened my_queue\n");

	//opening file
	fd = open(filename, O_RDONLY | O_LARGEFILE, S_IRUSR | S_IWUSR );


	while (file_size > bytes_already_read)
	{

		int data_size = min(MAX_QUEUE_MSG_SIZE, file_size - bytes_already_read);
		data = malloc(data_size);

		size_read = read(fd, data, data_size);

		/* Test for error */
		if( size_read == -1 )
		{
			perror( "Error reading myfile.dat" );
			exit(EXIT_FAILURE);
		}

		//sending message queue
		ret = mq_send(queue, data, size_read, prio);
		if (ret == -1)
		{
		   perror ("mq_send()");
		   exit(EXIT_FAILURE);
		}

		bytes_already_read += size_read;

		if (debug) printf("Data sent this loop: %d \n", size_read);
		if (debug) printf("Cumulated data sent: %ld over %ld\n", bytes_already_read, file_size);

		free(data);

		//looking at the queue state
		if (debug)
		{
			ret = mq_getattr (queue, &queueAttr);
			if (ret == -1) {
				perror ("mq_getattr()");
				exit(EXIT_FAILURE);
			}
			printf("Messages: %ld; send waits: %ld; receive waits: %ld\n\n", queueAttr.mq_curmsgs, queueAttr.mq_sendwait, queueAttr.mq_recvwait);
		}
	}

	//close the file
	ret = close(fd);
	if (ret !=0)
	{
		perror("fclose error");
		exit(EXIT_FAILURE);
	}

	ret = mq_close(queue);
	if (ret == -1)
	{
	 perror ("mq_close()");
	 exit(EXIT_FAILURE);
	}

	printf("All data sent with success\n");
	exit(EXIT_SUCCESS);
}
