//////////////////////////////////////////////////////////////////////////////////////////////
// The program should read a given file and send it to ipc_receivefile using a method that 	//
// is given as a command-line argument. 													//
//------------------------------------------------------------------------------------------//
// For the moment theses methods are implemented:											//
//		* message queue 																	//
// 		* message passing																	//
// 		* pipe 																				//
//		* share memory																		//
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
#include <sys/mman.h>
#include <sys/stat.h>
#include "copyfile.h"


struct option long_options[] =
{
	  {"help",     no_argument, NULL, 'h'},
	  {"message",  required_argument, NULL, 'm'},
	  {"queue",  required_argument, NULL, 'q'},
	  {"pipe",  required_argument, NULL, 'p'},
	  {"shm",    no_argument, NULL, 's'},
	  {"file",  required_argument, NULL, 'f'},
	  {0, 0, 0, 0}
};

int ipc_message(char filename[], char servername[]);
off_t findSize(char file_name[]);
void ipc_queue(char filename[], char queuename[]);
void ipc_shm(char filename[]);

char filename[MAXFILENAME];
char servername[MAXSERVERNAME];
char queuename[MAXQUEUENAME];
char shmName[]=SHARED_MEMORY_NAME;
iov_msg msg;
char* data;
int debug =1;

void unlink_and_exit(char *name)
{
	(void)shm_unlink(name);
	exit(EXIT_FAILURE);
}

int main (int argc, char *argv[])
{
	protocol_t protocol = NONE;
	int opt;
	while(1) //loop for taking care of arguments
	{
		int option_index=0; //getopt_long stores the option index here
		opt = getopt_long (argc, argv, "hm:q:p:sf:",long_options,&option_index);

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
				"	--shm to send the data to the receiver with a shared memory \n"
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
			printf("Protocol shared memory is chosen.\n");
			protocol = SHM;
			break;
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
			case SHM:
				if (strlen(filename)==0)
				{
					printf("Filename must be specified. Abort\n");
					return EXIT_FAILURE;
				}
				ipc_shm(filename);
				break;
			default:
				break;
		}


	return EXIT_SUCCESS;

}

int ipc_message(char filename[], char servername[])
{
	off_t file_size = findSize(filename);
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



off_t findSize(char file_name[])
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


void ipc_queue(char filename[], char queuename[])
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



void ipc_shm(char filename[])
{
	int fd;
	shmem_t *ptr;
	int ret;
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;
	int size_read = 1;



	//Opening share memory
	fd = shm_open(shmName, O_RDWR | O_CREAT | O_EXCL, 0660);
	if (fd == -1)
	{
		perror("shm_open()");
		unlink_and_exit(shmName);
	}

	/* set the size of the shared memory object, allocating at least one page of memory */
	ret = ftruncate(fd, sizeof(shmem_t));
	if (ret == -1)
	{
		perror("ftruncate");
		unlink_and_exit(shmName);
	}

	/* get a pointer to the shared memory */

	ptr = mmap(0, sizeof(shmem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED)
	{
		perror("mmap");
		unlink_and_exit(shmName);
	}

	/* don't need fd anymore, so close it */
	close(fd);

	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
	ret = pthread_mutex_init(&ptr->mutex, &mutex_attr);
	if (ret != EOK)
	{
		perror("pthread_mutex_init");
		unlink_and_exit(shmName);
	}

	pthread_condattr_init(&cond_attr);
	pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
	ret = pthread_cond_init(&ptr->cond, &cond_attr);
	if (ret != EOK)
	{
		perror("pthread_cond_init");
		unlink_and_exit(shmName);
	}

	/*
	 * our memory is now "setup", so set the init_flag
	 * it was guaranteed to be zero at allocation time
	 */
	ptr->init_flag = 1;

	printf("Shared memory created and init_flag set to let users know shared memory object is usable.\n");

	//opening file to read
	fd = open(filename, O_RDONLY | O_LARGEFILE, S_IRUSR | S_IWUSR );
/*
	while(1)
	{
		ret = pthread_mutex_lock(&ptr->mutex);
		if (ret != EOK)
		{
			perror("pthread_mutex_lock");
			unlink_and_exit(shmName);
		}

		if (ptr->data_version != 0)
			break;

		printf("Waiting for the receiver\n");
		ret = pthread_mutex_unlock(&ptr->mutex);
		if (ret != EOK)
		{
			perror("pthread_mutex_unlock");
			unlink_and_exit(shmName);
		}


		ret = pthread_cond_broadcast(&ptr->cond);
		if (ret != EOK)
		{
			perror("pthread_cond_broadcast");
			unlink_and_exit(shmName);
		}

		sleep(1);
	}*/

	while (size_read > 0) {
		sleep(1);

		/* lock the mutex because we're about to update shared data */
		ret = pthread_mutex_lock(&ptr->mutex);
		if (ret != EOK)
		{
			perror("pthread_mutex_lock");
			unlink_and_exit(shmName);
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
			unlink_and_exit(shmName);
		}

		/* wake up any readers that may be waiting */
		ret = pthread_cond_broadcast(&ptr->cond);
		if (ret != EOK)
		{
			perror("pthread_cond_broadcast");
			unlink_and_exit(shmName);
		}
	}

	sleep(1);
	/* unmap() not actually needed on termination as all memory mappings are freed on process termination */
	if (munmap(ptr, sizeof(shmem_t)) == -1)
	{
		perror("munmap");
	}

	/* but the name must be removed */
	if (shm_unlink(shmName) == -1)
	{
		perror("shm_unlink");
	}

	exit(EXIT_SUCCESS);

}
