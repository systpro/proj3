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
fat_struct **fat1;
root_struct **root;
int max_entries = 0;
int max_fat_entries = 0;

//TODO: read data from image and fill fat2
//TODO: compare contents of fat1 and fat2
//TODO: if contents are not the same inform user
//fat_struct *fat2;

int main()
{
    char fname[PATH_MAX];
    //the file descriptor for floppy image file
    int fd = 0;

    //command names
    char *help = "help\0";
    char *fmount = "fmount\0";
    char *umount = "umount\0";
    char *structure = "structure\0";
    char *showsector = "showsector\0";
    char *showfat = "showfat\0";
    char *traverse = "traverse\0";
    char *dashel = "-l\0";
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
            int fat_entries = 0;
            strcpy(fname, input[1]);
            fn_fmount(&fd, fname);
            //allocate heap storage for structs that hold organized image data.
            //@rj-pe TODO: allocate memory for FAT 2 struct.
            boot = (boot_struct*) malloc(sizeof(boot_struct));
            //read data from image file and copy into structs.
            //@rj-pe TODO: perform read and copy operations on FAT 2.
            read_boot(fd, boot);

            fat_entries = boot->num_sectors_fat * boot->num_bytes_per_sector / 3;
            if(fat_entries > max_fat_entries){
                max_fat_entries = fat_entries;
            }
            fat1 = (fat_struct **) malloc( fat_entries * sizeof(fat_struct *));
            for( int i = 0; i < fat_entries; i++){
                fat1[i] = malloc(sizeof(boot_struct));
            }
            read_fat(fd, boot, fat1);
            //keep track of max size of root to free correct amount of memory when closing
            if(boot->num_root_entries > max_entries){
                max_entries = boot->num_root_entries;
            }
            //allocate heap storage for root array
            root = (root_struct **) malloc(boot->num_root_entries * sizeof(root_struct *));
            for(int i = 0; i < boot->num_root_entries; i++){
                root[i] = malloc(sizeof(root_struct));
            }
            //read data from disk image into root struct array
            read_root(fd, boot, root);
        }
        //unmount the filesystem
        else if( strncmp(input[0], umount, 6) == 0){
            if( strlen(input[1]) != 0) {
                //check if given argument matches currently open fd
                if(strcmp(input[1], fname) == 0){
                    //match: proceed with umount
                    fn_umount(fd, fname);
                } else{ //no match: print error message
                    printf("%s %s\n%s", input[1],
                           "is not currently mounted.",
                           "hint: no argument is required for this command\n");
                }
            } else{
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
        //show contents of the first 256 entries in the FAT table
        else if(strncmp(input[0], showfat, 7) == 0){
            fn_showfat(fat1);
        }
        //list content in the root directory
        //TODO: function is working on a very basic level, add functionality
        else if(strncmp(input[0], traverse, 8) == 0){
            if(strncmp(input[1], dashel, 2) == 0){
                //fn_traverse_l();
            } else {
                //print_dir(root, boot->num_root_entries);
                fn_traverse(root, boot->num_root_entries, fat1);
            }
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
    //@rj-pe TODO: free all dynamically allocated mem (FAT2)
    free(boot);
    for(int i =0; i < max_fat_entries; i++){
        free(fat1[i]);
    }
    free(fat1);
    for(int i = 0; i < max_entries; i++){
        free(root[i]);
    }
    free(root);
    close(fd);
    exit( 0);
}