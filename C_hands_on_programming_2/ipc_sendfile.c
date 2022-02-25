//////////////////////////////////////////////////////////////////////////////////////////////
// The program should read a given file and send it to ipc_receivefile using a method that 	//
// is given as a command-line argument. 													//
//------------------------------------------------------------------------------------------//
// The different methods accepted will be:													//
//		* share memory																		//
//------------------------------------------------------------------------------------------//
// For the moment theses methods are implemented:											//
//		* message queue 																	//
// 		* message passing																	//
// 		* pipe 																				//
//------------------------------------------------------------------------------------------//
// This file should take as argument:														//
//		* --help to print out a help text containing short description of all supported		//
//			command line arguments															//
//		* --file <filename> file used to read data											//
//		* --<method> <element to connect to the receiver in asked> to give the method use 	//
//			 and the way to recognize the receiver.											//
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
#include <limits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "ipc_common_file.h"



int ipc_message(char * filename);
off_t findSize(char * file_name);
void ipc_queue(char* filename);
void ipc_pipe(char* filename);
void ipc_shm(char* filename);

char* filename;
int debug = DEBUG_VALUE;

void unlink_and_exit(char *name)
{
	(void)shm_unlink(name);
	exit(EXIT_FAILURE);
}


int main (int argc, char *argv[])
{
	arguments argumentsProvided = analyseArguments(argc,argv);

	switch (argumentsProvided.protocol) //launching correct function
		{
			case NONE:
				printf("Error. Missing arguments or wrong arguments. Use --help to know which arguments you can use\n");
				return EXIT_FAILURE;
				break;
			case HELP:
				break;
			case MSG:
				ipc_message(argumentsProvided.filename);
				break;
			case QUEUE:
				if (debug) printf("Launching ipc_queue protocol.\n");
				ipc_queue(argumentsProvided.filename);
				break;
			case PIPE:
				ipc_pipe(argumentsProvided.filename);
				break;
			case SHM:
				if (strlen(argumentsProvided.filename)==0)
				{
					printf("Filename must be specified. Abort\n");
					return EXIT_FAILURE;
				}
				ipc_shm(argumentsProvided.filename);
				break;
			default:
				break;
		}


	return EXIT_SUCCESS;

}

int ipc_message(char* filename)
{
	iov_msg msg;
	off_t file_size = findSize(filename);
	int coid = -1;
	iov_t siov[2];
	int fd;
	int status;
	int bytesRemaining = file_size;


	//locate or wait the server
	while (coid == -1)
	{
		coid = name_open(INTERFACE_NAME,0);
		printf("Waiting for the server.\n");
		sleep(2);
	}

	fd = open(filename, O_RDONLY | O_LARGEFILE, S_IRUSR | S_IWUSR );

	char* data = calloc(MAX_MSG_BUFF_SIZE,sizeof(char));
	if (debug) printf("Allocating %d bytes.\n", MAX_MSG_BUFF_SIZE);

	while (bytesRemaining > 0)
	{

		msg.data_size = read(fd, data, MAX_MSG_BUFF_SIZE);
		if( msg.data_size == -1 )
		{
			perror( "Error reading the file");
			free(data);
			exit(EXIT_FAILURE);
		}

		if (debug) printf("%lu data read.\n", msg.data_size);

		//Set the header
		msg.msg_type = CPY_IOV_MSG_TYPE;

		SETIOV(&siov[0], &msg, sizeof(msg));
		SETIOV(&siov[1], data, msg.data_size);

		printf("sending data...");
		if (debug) printf("Send a msg with type: %d\n", msg.msg_type);
		status = MsgSendvs(coid, siov, 2, NULL, 0);

		if (status == -1)
		{ //was there an error sending to server?
			perror("MsgSend");
			free(data);
			exit(EXIT_FAILURE);
		}
		bytesRemaining -= msg.data_size;
	}
	if (debug) printf("liberate the buffer\n");

	free(data);

	if (debug) printf("all data sent\n");
	close(fd);

	printf("All done. Closing the app.\n");
	return EXIT_SUCCESS;

}





void ipc_queue(char* filename)
{
	off_t file_size = findSize(filename);
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
	queue = mq_open(INTERFACE_NAME, O_WRONLY , S_IRWXU | S_IRWXG, &queueAttr);

	// If no queue -> the receiver is not launched yet... waiting for it
	while (queue == -1)
	{
		if (errno == ENOENT)
		{
			printf("Waiting for the receiver to connect to the queue\n");
			queue = mq_open(INTERFACE_NAME, O_WRONLY , S_IRWXU | S_IRWXG, &queueAttr);
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
	int data_size = min(MAX_QUEUE_MSG_SIZE, file_size);
	data = malloc(data_size);

	while (file_size > bytes_already_read)
	{
		size_read = read(fd, data, data_size);

		/* Test for error */
		if( size_read == -1 )
		{
			free(data);
			perror( "Error reading the file" );
			exit(EXIT_FAILURE);
		}

		//sending message queue
		ret = mq_send(queue, data, size_read, prio);
		if (ret == -1)
		{
			free(data);
		   perror ("mq_send()");
		   exit(EXIT_FAILURE);
		}

		bytes_already_read += size_read;

		if (debug) printf("Data sent this loop: %d \n", size_read);
		if (debug) printf("Cumulated data sent: %ld over %ld\n", bytes_already_read, file_size);



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
	free(data);
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

void ipc_pipe(char* filename)
{
	char * data;
	int size_read =1;
	int fd;
	int fifofd;

	//opening file
	fd = open(filename, O_RDONLY | O_LARGEFILE, S_IRUSR | S_IWUSR );

	while ( (fifofd = open(INTERFACE_NAME, O_WRONLY  | O_LARGEFILE, S_IRUSR | S_IWUSR )) == -1)
	{
		if (errno == ENOENT)
		{
			printf("Waiting for the receiver. Sleep(2)\n");
			sleep(2);
		}
		else
		{
			perror("fopen");
			exit(EXIT_FAILURE);
		}
	}

	data = malloc(PIPE_BUF);


	while(size_read != 0)
	{
		size_read = read(fd, data, PIPE_BUF); // reading the file
		if (debug) printf( "%d bytes read on the file\n", size_read);

		write(fifofd, data, size_read); // writing on the pipe
		if (debug) printf( "Successfully wrote in the pipe\n");
	}
	free(data);
	printf("Finished reading and sending data.\n");
	close(fifofd);
	close(fd);

}


void ipc_shm(char* filename)
{
	int fd;
	shmem_t *ptr;
	int ret;
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;
	int size_read = 1;

	//Opening share memory
	fd = shm_open(INTERFACE_NAME, O_RDWR | O_CREAT | O_EXCL, 0660);
	if (fd == -1)
	{
		perror("shm_open()");
		unlink_and_exit(INTERFACE_NAME);
	}

	/* set the size of the shared memory object, allocating at least one page of memory */
	ret = ftruncate(fd, sizeof(shmem_t));
	if (ret == -1)
	{
		perror("ftruncate");
		unlink_and_exit(INTERFACE_NAME);
	}

	/* get a pointer to the shared memory */

	ptr = mmap(0, sizeof(shmem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED)
	{
		perror("mmap");
		unlink_and_exit(INTERFACE_NAME);
	}

	/* don't need fd anymore, so close it */
	close(fd);

	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
	ret = pthread_mutex_init(&ptr->mutex, &mutex_attr);
	if (ret != EOK)
	{
		perror("pthread_mutex_init");
		unlink_and_exit(INTERFACE_NAME);
	}

	pthread_condattr_init(&cond_attr);
	pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
	ret = pthread_cond_init(&ptr->cond, &cond_attr);
	if (ret != EOK)
	{
		perror("pthread_cond_init");
		unlink_and_exit(INTERFACE_NAME);
	}

	/*
	 * our memory is now "setup", so set the init_flag
	 * it was guaranteed to be zero at allocation time
	 */
	ptr->init_flag = 1;

	printf("Shared memory created and init_flag set to let users know shared memory object is usable.\n");

	//opening file to read
	fd = open(filename, O_RDONLY | O_LARGEFILE, S_IRUSR | S_IWUSR );

	while (size_read > 0) {
		sleep(1);

		/* lock the mutex because we're about to update shared data */
		ret = pthread_mutex_lock(&ptr->mutex);
		if (ret != EOK)
		{
			perror("pthread_mutex_lock");
			unlink_and_exit(INTERFACE_NAME);
		}
		if (ptr->bothConnected > 0)
		{
			ptr->data_version++;
			size_read = read(fd, ptr->text, SHARE_MEMORY_BUFF);

			ptr->data_size = size_read;

		}
		else
			printf("Waiting for ipc_receivefile\n");


		/* finished accessing shared data, unlock the mutex */
		ret = pthread_mutex_unlock(&ptr->mutex);
		if (ret != EOK)
		{
			perror("pthread_mutex_unlock");
			unlink_and_exit(INTERFACE_NAME);
		}

		/* wake up any readers that may be waiting */
		ret = pthread_cond_broadcast(&ptr->cond);
		if (ret != EOK)
		{
			perror("pthread_cond_broadcast");
			unlink_and_exit(INTERFACE_NAME);
		}
	}

	sleep(1);
	/* unmap() not actually needed on termination as all memory mappings are freed on process termination */
	if (munmap(ptr, sizeof(shmem_t)) == -1)
	{
		perror("munmap");
	}

	/* but the name must be removed */
	if (shm_unlink(INTERFACE_NAME) == -1)
	{
		perror("shm_unlink");
	}

	printf("All data sent.\n");
	exit(EXIT_SUCCESS);
}

off_t findSize(char * file_name)
{
	struct stat statFile;
	int status;

	status = stat(file_name, &statFile);
	if (status == -1)
	{
		printf("stat\n");
		exit(EXIT_FAILURE);
	}
	return statFile.st_size;
}
