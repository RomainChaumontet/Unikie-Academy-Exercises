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
#include "copyfile.h"

typedef union
{
	uint16_t msg_type;
	struct _pulse pulse;
	iov_msg msg;;
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

char filename[MAXFILEMAME];
char servername[MAXSERVERNAME];
int debug = 0; // variable to see each step
send_by_msg msg;
FILE* fptr;

void ipc_message(char filename[], char servername[]);
int writing(char * data, char filename[], unsigned data_size);


int main (int argc, char *argv[])
{
	int method=-1;
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
			method=0;
			break;
		case 'f':
			if (strlen(optarg) > MAXFILEMAME)
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
			method=999;
			break;
		case '?':
			break;
		default:
			break;
		}

	}

	switch (method) //launching the correct function
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
				printf("ServerName must be specified. Abort\n");
				return EXIT_FAILURE;
			}
			ipc_message(filename, servername);
			break;
		default:
			break;
	}

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
				perror("MsgReceive, unknown type");
				exit(EXIT_FAILURE);
			}

			data = malloc(msg.msg.data_size);

			if (debug) printf("allocating %d bytes\n", msg.msg.data_size);
			if (data == NULL)
			{
				if (MsgError(rcvid, ENOMEM ) == -1)
				{
					perror("MsgError");
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				//Receive data and call writing function
				status = MsgRead(rcvid, data, msg.msg.data_size, sizeof(iov_msg));
				if (status == -1)
				{
					perror("MsgRead");
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

int writing(char * data, char filename[],unsigned data_size)
{
	if (debug) printf("Try writing the file %s with %u bytes\n", filename, data_size);
	int status;
	//write the text
	status = fwrite(data, data_size, 1, fptr);
	if (debug) printf( "Successfully wrote %d records\n", status );



	return EXIT_SUCCESS;

}

/*
 * ipc_receivefile.c
 *
 *  Created on: Feb 15, 2022
 *      Author: romain
 */


