

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/neutrino.h>
#include <getopt.h>
#include <sys/dispatch.h>
#include "ipc_common_file.h"



struct option long_options[] =
{
	  {"help",     no_argument, NULL, 'h'},
	  {"message",  no_argument, NULL, 'm'},
	  {"queue",  no_argument, NULL, 'q'},
	  {"pipe",  no_argument, NULL, 'p'},
	  {"shm",    no_argument, NULL, 's'},
	  {"file",  required_argument, NULL, 'f'},
	  {0, 0, 0, 0}
};

arguments analyseArguments(int argc, char *argv[])
{
	arguments retval;
	retval.protocol = NONE;
	int opt;

	while(1)
	{
		int option_index=0; //getopt_long stores the option index here
		opt = getopt_long (argc, argv, "hmqpsf:",long_options,&option_index);

		if (opt == -1) //no more options
			break;

		switch (opt)
		{
		case 'h':
			printf
			(
				"Ipc_sendfile and ipc_receivefile are programs which read a file, send data to the other program which write the data in another file.\n"
				"The program accept the following arguments:\n"
				"	--help to print this information\n"
				"	--message to exchange data by IPC message passing\n"
				"	--queue to exchange data by IPC queue #not yet implemented\n"
				"	--pipe to exchange data by IPC pipe \n"
				"	--shm to exchange data with a shared memory #not yet implemented\n"
				" 	--file <filename> to specify the filename which has to be write\n"
			);
			retval.protocol=HELP;
			break;
		case 'f':
			retval.filename = optarg;
			printf("The name of the file is %s\n",retval.filename);
			break;
		case 'm':
			printf("The message passing protocol has been chosen.\n");
			retval.protocol=MSG;
			break;
		case 'q':
			printf("The queue protocol has been chosen.\n");
			retval.protocol = QUEUE;
			break;
		case 'p':
			retval.protocol = PIPE;
			printf("The pipe protocol has been chosen.\n");
			break;
		case 's':
			retval.protocol = SHM;
			printf("Shared memory procotol is chosen \n");
			break;
		case '?':
			break;
		default:
			break;
		}

	}

	return retval;

}


