#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h> 


int main(int argc, char *argv[])
{
    int opt = 0, nsec = 2;
    short term = 0, spval = 0;
    // Collect opts
    while ((opt = getopt(argc,argv,"+n:be")) != -1)
    {    
        switch(opt)
        {
            case 'n':
                nsec = atoi(optarg);
                // Negative number of seconds:
                if (nsec <0)
                {
                    perror("Number of seconds cannot be negative.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'b':
                spval = 1;
                break;
            case 'e':
                term = 1;
                break;
            default:
                break;
        }
    }
    // Collect name of program to execute and list of arguments for it
    char *nargs[argc-optind+2];
    if (optind<argc)
    {
        for (int i=optind;i<argc;i++)
        {
            nargs[i-optind] = argv[i];
        }
        nargs[argc-optind] = '\0';
    }
    else 
    {
        // No program is provided
        perror("Arguments are not provided\n");
        exit(EXIT_FAILURE);
    }
    while (1)
    {     
        // fork the procces
        int pid = fork();
        if (pid < 0)
        {
            //no process is created:
            perror("Error while forking.\n");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // For child process execute
            int proc = execvp(nargs[0],nargs);
            if (proc <0)
            {
                //execution wasn't successful
                if (spval)
                {
                    perror("a\n");
                }
                else
                {
                    perror("Error starting a program");
                }
                //return failure to parent
                _exit(EXIT_FAILURE);
            }
            //return success
            _exit(EXIT_SUCCESS);
        }
        else if (pid > 0)
        {
            //for parent we await the result
            int status = 0;
            (void)waitpid(pid,&status, 0);
            if (status&& term)
            {

                exit(EXIT_FAILURE);
            }
        }
        //sleep for give amount of seconds (default 2)
         sleep(nsec);
    }
    
    return 1;
}