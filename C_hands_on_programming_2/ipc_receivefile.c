//////////////////////////////////////////////////////////////////////////////////////////////
// The program should receive data from the ipc_sendfile program and write it in a file
//------------------------------------------------------------------------------------------
// For the moment theses methods are implemented:
// 		* message passing
//		* message queue
// 		* pipe
//		* share memory
//------------------------------------------------------------------------------------------
// This file should take as argument:
//		* --help to print out a help text containing short description of all supported
//			command line arguments
//		* --file <filename> file used to write data
//		* --<method> <element to connect to the sender> to give the method use and the
//			way to recognize the sender.
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
#include <sys/mman.h>
#include <sys/stat.h>
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
	  {"shm",    no_argument, NULL, 's'},
	  {"pipe",  no_argument, NULL, 'p'},
	  {"file",  required_argument, NULL, 'f'},
	  {0, 0, 0, 0}
};

char filename[MAXFILENAME];
char servername[MAXSERVERNAME];
char queuename[MAXQUEUENAME];
char shmName[]=SHARED_MEMORY_NAME;
char pipeName[] = PIPE_NAME;
int debug = 1; // variable to see each step
send_by_msg msg;
FILE* fptr;

void ipc_message(char filename[], char servername[]);
void ipc_queue(char filename[], char queuename[]);
void ipc_shm(char filename[]);
int writing(char * data, char filename[], unsigned data_size);
void *get_shared_memory_pointer( char *name, unsigned num_retries );

void ipc_pipe(char filename[], char pipeName[]);


int main (int argc, char *argv[])
{
	protocol_t protocol = NONE;
	int opt;
	while(1)
	{
		int option_index=0; //getopt_long stores the option index here
		opt = getopt_long (argc, argv, "hm:q:psf:",long_options,&option_index);

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
				"	--queue <Choose_your_queue_name> to receive the data from the sender by IPC queue #not yet implemented\n"
				"	--pipe to receive the data from the sender by IPC pipe \n"
				"	--shm to receive the data from the sender with a shared memory #not yet implemented\n"
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
			protocol = PIPE;
			printf("The pipe protocol has been chosen.\n");
			break;
		case 's':
			protocol = SHM;
			printf("Shared memory procotol is chosen \n");
			break;
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
		case SHM:
      if (strlen(filename)==0)
			{
				printf("Filename must be specified. Abort\n");
				return EXIT_FAILURE;
			}
			ipc_shm(filename);
      break;
		case PIPE:
			if (strlen(filename)==0)
			{
				printf("Filename must be specified. Abort\n");
				return EXIT_FAILURE;
			}
			ipc_pipe(filename, pipeName);
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


void ipc_shm(char filename[])
{
	int ret;
	shmem_t *ptr;
	uint8_t continueLoop = 1; // Become 0 if there is no more data to read.
	int last_version =0;


	/* try to get access to the shared memory object, retrying for 100 times (100 seconds) */
	ptr = get_shared_memory_pointer(shmName, 100);
	if (ptr == MAP_FAILED)
	{
		fprintf(stderr, "Unable to access object '%s' - was creator run with same name?\n", shmName);
		exit(EXIT_FAILURE);
	}

	fptr = fopen64(filename, "wb");  //Create/open the file in write binary mode


	//receiving data
	while (continueLoop == 1)
	{
		/* lock the mutex because we're about to access shared data */
		ret = pthread_mutex_lock(&ptr->mutex);
		if (ret != EOK)
		{
			perror("pthread_mutex_lock");
			fclose(fptr);
			exit(EXIT_FAILURE);
		}

		/* wait for changes to the shared memory object */
		while (last_version == ptr->data_version) {
			if (ptr->bothConnected == 0)
			{
				ptr->bothConnected = 1; //Signaling sender can start tranfer data
			}
			ret = pthread_cond_wait(&ptr->cond, &ptr->mutex); /* does an unlock, wait, lock */
			if (ret != EOK)
			{
				perror("pthread_cond_wait");
				fclose(fptr);
				exit(EXIT_FAILURE);
			}
		}

		/* update local version and data */
		last_version = ptr->data_version;
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
	}

	fclose(fptr);
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

void ipc_pipe(char filename[], char pipeName[])
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

	status = mkfifo(pipeName, S_IRWXU | S_IRWXG);
	if (status == -1 && errno != EEXIST)
	{
		perror("mkfifo");
		exit(EXIT_FAILURE);
	}

	fd = open(pipeName,O_RDONLY);

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

	status = remove(pipeName);
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


