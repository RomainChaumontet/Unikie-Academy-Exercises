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
#include <limits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "ipc_common_file.h"

char * filename;
int debug = DEBUG_VALUE; // variable to see each step
FILE* fptr;

void ipc_message(char * filename);
void ipc_queue(char * filename);
int writing(char * data, char * filename, unsigned data_size);
void ipc_pipe(char * filename);
void ipc_shm(char* filename);
void *get_shared_memory_pointer( char *name, unsigned num_retries );


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

int writing(char * data, char * filename,unsigned data_size)
{
	int ret;
	if (debug) printf("Try writing the file %s with %u bytes\n", filename, data_size);

	//write the text
	ret = fwrite(data, data_size, 1, fptr);
	if (ret == -1)
	{
		perror("writing file");
		return EXIT_FAILURE;
	}

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
			status = writing(data, filename, msg.msg.data_size);
			if (status == -1)
			{
				free(data);
				exit(EXIT_FAILURE);
			}
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
	ret = writing(data, filename, bytes_received);
	if (ret == -1)
	{
		free(data);
		exit(EXIT_FAILURE);
	}
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
		ret = writing(data, filename, bytes_received);
		if (ret == -1)
		{
			free(data);
			exit(EXIT_FAILURE);
		}

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

	data = malloc(PIPE_BUF);
	while (size_read == 0 ) //waiting for data on the pipe
	{
		sleep(1);
		size_read = read(fd, data, PIPE_BUF);
	}
	status = writing(data, filename, size_read);
	if (status == -1)
	{
		free(data);
		exit(EXIT_FAILURE);
	}
	if (debug) printf("%d bytes written on the file\n", size_read);


	while(size_read > 0)
	{
		size_read = read(fd, data, PIPE_BUF);
		status = writing(data, filename, size_read);
		if (status == -1)
		{
			free(data);
			exit(EXIT_FAILURE);
		}
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

void ipc_shm(char* filename)
{
	int ret;
	shmem_t *ptr;
	uint8_t continueLoop = 1; // Become 0 if there is no more data to read.
	sem_t* semaphorePtr;
	const char * semName = SEMAPHORE_NAME;


	/* try to get access to the shared memory object, retrying for 100 times (100 seconds) */
	ptr = get_shared_memory_pointer(INTERFACE_NAME, 100);
	if (ptr == MAP_FAILED)
	{
		fprintf(stderr, "Unable to access object '%s' - was creator run with same name?\n", INTERFACE_NAME);
		exit(EXIT_FAILURE);
	}

	fptr = fopen64(filename, "wb");  //Create/open the file in write binary mode

	//Opening Semaphore
	semaphorePtr = sem_open(semName, 0);
	while (semaphorePtr == SEM_FAILED)
	{
		if (errno == EACCES)
		{
			printf("The semaphore is not create for the moment. Waiting for ipc_sendfile.\n");
			sleep(1);
			semaphorePtr = sem_open(semName, 0);
		}
		else
		{
			perror("sem_open");
			exit(EXIT_FAILURE);
		}
	}

	// Launching Semaphore for ipc_sendfile
	ret = sem_post(semaphorePtr);
	if (ret == -1)
	{
		perror("sem_post");
		exit(EXIT_FAILURE);
	}


	//receiving data
	while (continueLoop == 1)
	{
		ret = sem_wait(semaphorePtr);
		if (ret == -1)
		{
			perror("sem_wait");
			exit(EXIT_FAILURE);
		}
		/* lock the mutex because we're about to access shared data */
		ret = pthread_mutex_lock(&ptr->mutex);
		if (ret != EOK)
		{
			perror("pthread_mutex_lock");
			fclose(fptr);
			exit(EXIT_FAILURE);
		}

		/* update local version and data */
		writing(ptr->text, filename, ptr->data_size);

		if (ptr->data_size == 0) //no more data to write -> stop the loop after unlocking the mutex.
			continueLoop = 0;

		/* finished accessing shared data, unlock the mutex */
		ret = pthread_mutex_unlock(&ptr->mutex);
		if (ret != EOK)
		{
			perror("pthread_mutex_unlock");
			fclose(fptr);
			exit(EXIT_FAILURE);
		}

		ret = sem_post(semaphorePtr);
		if (ret == -1)
		{
			perror("sem_post");
			exit(EXIT_FAILURE);
		}
	}

	fclose(fptr);

	printf("All data received.\n");

	ret = sem_close(semaphorePtr);
	if (ret == -1)
	{
		perror("sem_close");
		exit(EXIT_FAILURE);
	}

	/* unmap() not actually needed on termination as all memory mappings are freed on process termination */
	if (munmap(ptr, sizeof(shmem_t)) == -1)
	{
		perror("munmap");
	}

	exit(EXIT_SUCCESS);

}

void *get_shared_memory_pointer( char *name, unsigned num_retries )
{
	unsigned tries;
	shmem_t *ptr;
	int fd;

	for (tries = 0;;) {
		fd = shm_open(name, O_RDWR, 0);
		if (fd != -1) break;
		++tries;
		if (tries > num_retries) {
			perror("shmn_open");
			return MAP_FAILED;
		}
		/* wait one second then try again */
		sleep(1);
	}

	for (tries = 0;;) {
		ptr = mmap(0, sizeof(shmem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (ptr != MAP_FAILED) break;
		++tries;
		if (tries > num_retries) {
			perror("mmap");
			return MAP_FAILED;
		}
		/* wait one second then try again */
		sleep(1);
	}

	/* no longer need fd */
	(void)close(fd);

	for (tries=0;;) {
		if (ptr->init_flag) break;
		++tries;
		if (tries > num_retries) {
			fprintf(stderr, "init flag never set\n");
			(void)munmap(ptr, sizeof(shmem_t));
			return MAP_FAILED;
		}
		/* wait on second then try again */
		sleep(1);
	}

	return ptr;
}

