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
file_struct **files;
int max_entries = 0;
int max_fat_entries = 0;
int max_files = 0;

//TODO: read data from image and fill fat2
//TODO: compare contents of fat1 and fat2
//TODO: if contents are not the same inform user
//fat_struct *fat2;

int main()
{
    char fname[PATH_MAX];
    //the file descriptor for floppy image file
    int fd = 0;
    int data_start = 0;
    int fat_entries = 0;
    int root_entries = 0;
    int file_count = 0;
    int cluster_bytes = 0;

    //command names
    char *help = "help\0";
    char *fmount = "fmount\0";
    char *umount = "umount\0";
    char *structure = "structure\0";
    char *showsector = "showsector\0";
    char *showfat = "showfat\0";
    char *traverse = "traverse\0";
    char *dashel = "-l\0";
    char *showfile = "showfile\0";
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
        /////////////////////////
        ///display help message//
        /////////////////////////
        if( strncmp(input[0], help, 4) == 0 ){
            fn_help();
        }
        /////////////////////////
        ///mount the filesystem//
        /////////////////////////
        else if( strncmp(input[0], fmount, 6) == 0){
            //TODO: check if fd has already been set, if it is return an error message.
            strcpy(fname, input[1]);
            fn_fmount(&fd, fname);
            ///sequence of operations for each structure is as follows
            ///allocate heap storage for struct.
            ///read data from image file, organize and copy it into struct.
            //@rj-pe TODO: allocate memory for FAT 2 struct.
            //@rj-pe TODO: perform read and copy operations on FAT 2.

            ///////////////
            ///boot struct/
            ///////////////
            //allocate heap storage for boot
            boot = (boot_struct*) calloc(1, sizeof(boot_struct));
            //read data from disk image into boot struct
            read_boot(fd, boot);

            //////////////
            ///fat_struct/
            //////////////
            //keep track of max size of fat to free correct amount of memory when closing
            fat_entries = boot->num_sectors_fat * boot->num_bytes_per_sector / 3;
            if(fat_entries > max_fat_entries){
                max_fat_entries = fat_entries;
            }
            //allocate heap storage for fat
            fat1 = (fat_struct **) malloc( fat_entries * sizeof(fat_struct *));
            //allocate heap storage for fat array
            for( int i = 0; i < fat_entries; i++){
                fat1[i] = malloc(sizeof(boot_struct));
            }
            //read data from disk image into fat struct
            read_fat(fd, boot, fat1);

            ///////////////
            ///root_struct/
            ///////////////
            //keep track of max size of root to free correct amount of memory when closing
            root_entries = boot->num_root_entries;
            if(root_entries > max_entries){
                max_entries = root_entries;
            }
            //allocate heap storage for root array
            root = (root_struct **) malloc(root_entries * sizeof(root_struct *));
            for(int i = 0; i < root_entries; i++){
                root[i] = calloc(1, sizeof(root_struct));
            }
            //read data from disk image into root struct array
            read_root(fd, boot, root);

            ///////////////
            ///file_struct/
            ///////////////
            //TODO: ensure that this calculation for the start of the data sector is correct

            data_start = (boot->num_sectors_fat * boot->num_fat + boot->num_reserved_sectors + (root_entries * 32)/boot->num_bytes_per_sector * 1) * boot->num_sectors_per_cluster;
            cluster_bytes = boot->num_sectors_per_cluster * boot->num_bytes_per_sector;
            //keep track of max size of files to free correct amount of memory when closing
            if((file_count = get_file_count(root, root_entries)) > max_files){
                max_files = file_count;
            }
            //allocate heap storage for files array
            files = (file_struct **) malloc(file_count * sizeof(file_struct));
            for(int i = 0; i < file_count; i++){
                files[i] = malloc(sizeof(file_struct));
            }
            //read data from disk image into files array
            int files_alloced = read_files(fd, root_entries, data_start, cluster_bytes, files, root, fat1);
            if( files_alloced != file_count){
               printf("allocation error: %d\n", max_files - files_alloced);
            }
        }
        ///////////////////////////
        ///unmount the filesystem//
        ///////////////////////////
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
        /////////////////////////////////////////
        ///display structure of the floppy disk//
        /////////////////////////////////////////
        else if(strncmp(input[0], structure, 9) == 0){
            fn_structure(boot);
        }
        /////////////////////////////////////////////////////
        ///print a hex dump of a given sector of the floppy//
        /////////////////////////////////////////////////////
        else if(strncmp(input[0], showsector, 10) == 0){
            //convert user input to a number
            char *next;
            long sector = strtol(input[1], &next, 10);
            //display the requested sector
            fn_showsector(fd, sector, boot);
        }
        ////////////////////////////////////////
        ///show first 256 entries in FAT table//
        ////////////////////////////////////////
        else if(strncmp(input[0], showfat, 7) == 0){
            fn_showfat(fat1);
        }
        ///////////////////////////////////////
        ///list content in the root directory//
        ///////////////////////////////////////
        //TODO: function is working on a very basic level, add functionality
        else if(strncmp(input[0], traverse, 8) == 0){
            if(strncmp(input[1], dashel, 2) == 0){
                //fn_traverse_l();
            } else {
                //print_dir(root, boot->num_root_entries);
                fn_traverse(root, root_entries, fat1, fd);
            }
        }
        /////////////////////////////////////////
        ///print a hex dump of a given filename//
        /////////////////////////////////////////
        else if(strncmp(input[0], showfile, 8) == 0){
            if(fn_showfile(files, file_count, input[1]) < 0){
                printf("file not found: %s\n", input[1]);
            }
        }
        /////////////////
        ///quit program//
        /////////////////
        else if( strncmp(input[0], quit, 4) == 0){
            break;
        }
        ////////////////////////////
        ///user pressed return key//
        ////////////////////////////
        else if( strncmp(input[0], "\n", 1) == 0){ continue;}
        ///////////////////////////////////////
        ///user did not enter a valid command//
        ///////////////////////////////////////
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
    for(int i = 0; i < max_files; i++){
        free(files[i]->cluster_list);
        free(files[i]->data);
    }
    for(int i = 0; i < max_files; i++){
        free(files[i]);
    }
    free(files);
    close(fd);
    exit( 0);
}