//////////////////////////////////////////////////////////////////////////////////////////////
// The program should read a given file and send it to ipc_receivefile using a method that 	//
// is given as a command-line argument. 													//
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

char filename[MAXFILENAME];
char servername[MAXSERVERNAME];
iov_msg msg;
char* data;
int debug =0;

int main (int argc, char *argv[])
{
	// variable method store the message used to send data:
	// 	-1 not specified => will cause an error
	//	0 help is invoked
	//	1 message method is invoked
	int method=-1;
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
				"	--queue <TBD> to send the data to the receiver by IPC queue #not yet implemented\n"
				"	--pipe <TBD> to send the data to the receiver by IPC pipe #not yet implemented\n"
				"	--shm <TBD> to send the data to the receiver with a shared memory #not yet implemented\n"
				" 	--file <filename> to specify the filename which has to be read\n"
			);
			method=0;
			break;
		case 'f':
			if (strlen(optarg) > MAXFILEMAME)
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
			method=1;
			break;
		case 'q':
		case 'p':
		case 's':
			printf("This option is not implemented yet. Use --help to know witch ones are\n");
			break;
		case '?':
			break;
		default:
			break;
		}

	}

	switch (method) //launching correct function
		{
			case -1:
				printf("Error. Missing arguments or wrong arguments. Use --help to know which arguments you can use\n");
				return EXIT_FAILURE;
				break;
			case 999:
			case 0:
				break;
			case 1:
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
			default:
				break;
		}


	return EXIT_SUCCESS;

}

int ipc_message(char filename[], char servername[])
{
	long int file_size = findSize(filename);
	long int bytes_already_read = 0;
	int coid = -1;
	iov_t siov[2];
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

	while (file_size > bytes_already_read)
	{
		char* buffer = malloc(4096);

		size_read = read(fd, buffer, 4096);

		/* Test for error */
		if( size_read == -1 )
		{
			perror( "Error reading myfile.dat" );
			return EXIT_FAILURE;
		}

		msg.data_size = size_read;

		SETIOV(&siov[0], &msg, sizeof(msg));
		SETIOV(&siov[1], buffer, size_read);

		if (debug) printf("sending one package of %d bytes\n",size_read);

		status = MsgSendvs(coid, siov, 2, NULL, 0);
		if (status == -1)
		{ //was there an error sending to server?
			perror("MsgSend");
			exit(EXIT_FAILURE);
		}
		bytes_already_read += size_read;

		if (debug) printf("Data sent: %ld over %ld\n", bytes_already_read, file_size);

		free(buffer);
	}
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
