#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <errno.h>
#include <zconf.h>
#include <stdlib.h>


#include "floppy.h"
/**
 * @brief Data from boot sector of floppy organized into a struct.
 *
 * By referring to this stuct, the program has access physical properties of the disk without reading them
 * directly from the floppy image.
 */
boot_struct *boot;

int main()
{
    //TODO: The filename & fd should not be hardcoded.
    char fname[PATH_MAX];
    //for debugging purposes, set the absolute pathname of the .img file below
    strcpy(fname, "../imagefile.img");
    //the file descriptor for floppy image file
    int fd = open(fname, O_RDONLY);

    if(fd < 0 ){
        perror(strerror(errno));
    }

    //command names
    char *help = "help\0";
    char *fmount = "fmount\0";
    char *umount = "umount\0";
    char *structure = "structure\0";
    char *showsector = "showsector\0";
    char *quit = "quit\0";
    while(1)
    {
        //input holds two strings of length 20
        char input[2][20];
        strncpy(input[0], "\0", 20);
        strncpy(input[1], "\0", 20);

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
                user_args = strtok(NULL, "\n");
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
            //TODO: check if fd has already been set, if it is return an error message.
            fn_fmount(fd, fname);
            //allocate heap storage for structs that hold organized image data.
            //@rj-pe TODO: allocate memory for FAT 1,FAT 2, and root structs.
            boot = (boot_struct*) malloc(sizeof(boot_struct));
            //read data from image file and copy into structs.
            //@rj-pe TODO: perform read and copy operations on FAT 1, FAT 2, and root.
            read_boot(fd, boot);
        }
        //unmount the filesystem
        else if( strncmp(input[0], umount, 6) == 0){
            if( strlen(input[1]) != 0) {
                //check if given argument matches currently open fd
                if(strcmp(input[1], fname) == 0){
                    //match: proceed with umount
                    fn_umount(fd, fname);
                }
                    //no match: print error message
                else{
                    printf("%s %s\n%s", input[1],
                           "is not currently mounted.",
                           "hint: no argument is required for this command\n");
                }
            }
            else{
                fn_umount(fd, fname);
            }
        }
        //display structure of the floppy disk
        else if(strncmp(input[0], structure, 9) == 0){
            fn_structure(boot);
        }
        //print a hex dump of a given sector of the floppy
        else if(strncmp(input[0], showsector, 10) == 0){
            //convert user input to a number
            char *next;
            long sector = strtol(input[1], &next, 10);
            //display the requested sector
            fn_showsector(fd, sector, boot);
        }
        //quit program
        else if( strncmp(input[0], quit, 4) == 0){
            break;
        }
        //user pressed return, do nothing
        else if( strncmp(input[0], "\n", 1) == 0){ continue;}
        //user did not enter a valid command
        else{printf("please re-try your command\n");}
    }
    //@rj-pe TODO: free all dynamically allocated mem (FAT1, FAT2, root)
    free(boot);
    close(fd);
    return 0;
}