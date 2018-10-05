#include <stdio.h>
#include <memory.h>
#include <string.h>
#include "floppy.h"

//description of function
void fn_help()
{
    printf("placeholder text\n");
}
//description of function
int fn_fmount(int fd)
{
    return 0;
}
//description of function
int fn_umount(int fd)
{
    return 0;
}

int main()
{
    //the file descriptor for floppy image file
    int fd;
    while(1)
    {
        //input holds two strings of length 20
        char input[2][20];
        strncpy(input[0], "\0", 20);
        strncpy(input[1], "\0", 20);

        //command names
        char *help = "help\0";
        char *fmount = "fmount\0";
        char *umount = "umount\0";
        char *quit = "quit\0";

        //the prompt
        printf("floppy:\t");

        //read entire line of user input
        char line[40];
        fgets(line, 40, stdin);

        //parse line into a command & an argument
        char * user_args;
        user_args = strtok(line, " ");
        for(int i = 0; i < 2 ; ++i) {
            if(user_args == NULL){break;}
            if(strlen(user_args) <= 20) {
                strncpy(input[i], user_args, strlen(user_args));
                user_args = strtok(NULL, " ");
            }
            //are floppy image names going to be extraordinarily long?
            else{printf("command exceeds maximum input length\n");}
        }

        //display help message
        if( strncmp(input[0], help, 4) == 0 ){
            fn_help();
        }
        //mount the filesystem
        else if( strncmp(input[0], fmount, 6) == 0){
            fn_fmount(fd);
        }
        //unmount the filesystem
        else if( strncmp(input[0], umount, 6) == 0){
            if( *input[1] != '\0') {
                //search for fd of input[1]
                fn_umount(fd);
            }
            else{
                fn_umount(fd);
            }
        }
        //quit program
        else if( strncmp(input[0], quit, 4) == 0){
            break;
        }
        else{printf("please re-try your command\n");}
    }
    return 0;
}