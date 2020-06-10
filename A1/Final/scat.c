#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h> 


int main(int argc, char* argv[])
{
    //read arguments
    short loop = 0, opt;
    while((opt = getopt(argc, argv, "lsp")) != -1){
        switch(opt){
            case 'l':
            loop = 0;
            break;
            case 's':
            loop = 1;
            break;
            case 'p':
            loop = 2;
            break;
            default:
            loop = 0;
        }
    }

    if (loop == 0)
    {
        // library loop :
        int c =0;
        while ((c=getc(stdin))!=EOF)
        {
            if (putc(c,stdout) == EOF)
            {   
                perror("Error writing to stdout.\n");
                exit(1);
            }
        }
    }
    if (loop == 1)
    {
        // system call loop:
        int ret = 0;
        char c;
        while ((ret=read(0,&c,1))>0){
            if (write(1,&c,1)<0)
            {
                perror("Error writing to stdout.\n");
                exit(1);
            }
        }
        if (ret<0)
        {    
            perror("Error reading the stdin.\n");
            exit(1);
            
        }
    }

    if (loop == 2)
    {
        //sendfile loop:
        int stat = 0;
        while (1){
            stat = sendfile(1,0,0,4096);
            if (stat == 0)
            {
                break;
            }
            if (stat < 0)
            {
                perror("Error reading the stdin.\n");
                exit(1);
            }
        }
    }


       
    return 0;

}