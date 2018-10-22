#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <errno.h>
#include <zconf.h>
#include <stdlib.h>
#include <sys/types.h>


#include "floppy.h"
#include "data_structures.h"

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
stack directories;
int dirs_visited = 0;
off_t arr_dir_visited[32];

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
    char mount_flag = 0;
    int max[3];
    directories.top = 0;
    directories.max_size = 32;

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
        //////////////////////////
        /// display help message//
        //////////////////////////
        if( strncmp(input[0], help, 4) == 0 ){
            fn_help();
        }
        //////////////////////////
        /// mount the filesystem//
        //////////////////////////
        else if( strncmp(input[0], fmount, 6) == 0){
            //TODO: check if fd has already been set, if it is return an error message.
            if(fd != 0){
                printf("Image, %s, is mounted. Please run umount!\n", fname);
                continue;
            }
            strcpy(fname, input[1]);
            //error checking for valid filename
            if(fn_fmount(&fd, fname) == 0){//user entered valid filename, process the image.
                ;
            } else{ //file not found, reprint floppy prompt.
                continue;
            }
            ///sequence of operations for each structure is as follows
            ///allocate heap storage for struct.
            ///read data from image file, organize and copy it into struct.
            //@rj-pe TODO: allocate memory for FAT 2 struct.
            //@rj-pe TODO: perform read and copy operations on FAT 2.

            ////////////////
            /// boot struct/
            ////////////////
            //allocate heap storage for boot
            boot = (boot_struct*) calloc(1, sizeof(boot_struct));
            //read data from disk image into boot struct
            read_boot(fd, boot);

            ///////////////
            /// fat_struct/
            ///////////////
            //keep track of size of fat to free correct amount of memory when umounting
            fat_entries = boot->num_sectors_fat * boot->num_bytes_per_sector / 3;
            max[0] = fat_entries;

            //allocate heap storage for fat
            fat1 = (fat_struct **) malloc( fat_entries * sizeof(fat_struct *));
            //allocate heap storage for fat array
            for( int i = 0; i < fat_entries; i++){
                fat1[i] = malloc(sizeof(boot_struct));
            }
            //read data from disk image into fat struct
            read_fat(fd, boot, fat1);

            ////////////////
            /// root_struct/
            ////////////////
            //keep track of size of root to free correct amount of memory when umounting
            root_entries = boot->num_root_entries;
            max[1] = root_entries;
            //allocate heap storage for root array
            root = (root_struct **) malloc(root_entries * sizeof(root_struct *));
            for(int i = 0; i < root_entries; i++){
                root[i] = calloc(1, sizeof(root_struct));
            }
            //read data from disk image into root struct array
            read_root(fd, boot, root, NULL);

            ////////////////
            /// file_struct/
            ////////////////
            //calculate where the data sector starts
            data_start =
                    ( boot->num_sectors_fat * boot->num_fat
                     + boot->num_reserved_sectors + (root_entries * 32)
                     / boot->num_bytes_per_sector * 1 )
                     * boot->num_sectors_per_cluster;
            //calculate number of bytes per cluster
            cluster_bytes = boot->num_sectors_per_cluster * boot->num_bytes_per_sector;
            //keep track of size of files to free correct amount of memory when umounting
            file_count = get_file_count(root, root_entries);
            max[2] = file_count;
            //allocate heap storage for files array
            files = (file_struct **) malloc(file_count * sizeof(file_struct));
            for(int i = 0; i < file_count; i++){
                files[i] = malloc(sizeof(file_struct));
            }
            //read data from disk image into files array
            int files_alloced
                    = read_files(fd, root_entries, data_start, cluster_bytes, NULL, files, root, fat1);
            if( files_alloced != file_count){
               printf("allocation error: %d\n", file_count - files_alloced);
            }
            ////////////////////////////////
            /// Process nested directories/
            ////////////////////////////////
            // Add directory entries to stack.
            char abs_path[12];
            if(add_dirs_to_stack(&directories, files, abs_path, file_count)){
                off_t next = pop(&directories);
                for(; next > 0; next = pop(&directories)){
                    // Process each directory's image data.

                    // Figure out how many root entries to add.
                    int init_root_count = root_entries;
                    root_entries += count_root_entries(fd, next, cluster_bytes);
                    max[1] = root_entries;
                    int add_entries = root_entries - init_root_count;
                    // Allocate for requisite number of root structs.
                    root_struct **root_canary = realloc(root, root_entries * sizeof(root_struct *));
                    if(root_canary != NULL){
                        root = root_canary;
                        for(int i = init_root_count; i < root_entries; i++){
                            root[i] = calloc(1, sizeof(root_struct));
                        }
                    }
                    // Parse dir image data into root structs.
                    off_t dir_flag[3] = {next, add_entries , init_root_count};
                    read_root(fd, boot, root, dir_flag);
                    // Prepend absolute path to filename
                    prepend_abs_path(root, abs_path, init_root_count, root_entries);

                    // Parse new root structs into file structs.
                    int init_file_count = file_count;
                    file_count = get_file_count(root, root_entries);
                    max[2] = file_count;
                    // Allocate heap storage for new file structs.
                    file_struct **files_canary = (file_struct **) realloc(files, file_count * sizeof(file_struct));
                    if(files_canary != NULL){
                        files = files_canary;
                    }
                    for(int i = init_file_count; i < file_count; i++){
                        files[i] = malloc(sizeof(file_struct));
                    }
                    // Read data from disk image into files array.
                    dir_flag[1] = init_file_count;
                    files_alloced
                            = read_files(
                                    fd, root_entries, data_start, cluster_bytes, dir_flag, files, root, fat1);
                    if( files_alloced != file_count){
                        printf("allocation error: %d\n", file_count - files_alloced);
                    }
                    // Add directory to visited list
                    arr_dir_visited[dirs_visited++] = next;
                    // Look for new directories
                    add_dirs_to_stack(&directories, files, abs_path, file_count);
                }
            }
            mount_flag = 1;
        }
        ////////////////////////////
        /// unmount the filesystem//
        ////////////////////////////
        else if( strncmp(input[0], umount, 6) == 0){
            if( strlen(input[1]) != 0) {
                //check if given argument matches currently open fd
                if(strcmp(input[1], fname) == 0 && mount_flag){
                    //match: proceed with umount
                    fn_umount(&fd, fname, max, boot, fat1, root, files);
                } else{ //no match: print error message
                    printf("%s %s\n%s", input[1],
                           "is not currently mounted.",
                           "hint: no argument is required for this command\n");
                }
            }
            else if(mount_flag)
            {
                fn_umount(&fd, fname, max, boot, fat1, root, files);
            } else{
                printf("no image mounted!\n");
            }
        }
        ////////////////////////////////////
        /// display floppy disk structure//
        ///////////////////////////////////
        else if(strncmp(input[0], structure, 9) == 0){
            fn_structure(boot);
        }
        //////////////////////////////////////////////////////
        /// print a hex dump of a given sector of the floppy//
        //////////////////////////////////////////////////////
        else if(strncmp(input[0], showsector, 10) == 0){
            //convert user input to a number
            char *next;
            long sector = strtol(input[1], &next, 10);
            //display the requested sector
            fn_showsector(fd, sector, boot);
        }
        //////////////////////////////////////////
        /// print first 256 entries in FAT table//
        //////////////////////////////////////////
        else if(strncmp(input[0], showfat, 7) == 0){
            fn_showfat(fat1);
        }
        ////////////////////////////////////////
        /// list content of the root directory//
        ////////////////////////////////////////
        //TODO: function is working on a very basic level, add functionality
        else if(strncmp(input[0], traverse, 8) == 0){
            if(strncmp(input[1], dashel, 2) == 0){
                fn_traverse(root, root_entries, fat1, fd, 1);
            } else {
                fn_traverse(root, root_entries, fat1, fd, 0);
            }
        }
        //////////////////////////////////////////
        /// print a hex dump of a given filename//
        //////////////////////////////////////////
        else if(strncmp(input[0], showfile, 8) == 0){
            if(fn_showfile(files, file_count, input[1]) < 0){
                printf("file not found: %s\n", input[1]);
            }
        }
        //////////////////
        /// quit program//
        //////////////////
        else if( strncmp(input[0], quit, 4) == 0){
            if(fd != 0 && mount_flag){
                //if an image remains mounted, call umount now.
                fn_umount(&fd, fname, max, boot, fat1, root, files);
            }
            break;
        }
        /////////////////////////////
        /// user pressed return key//
        /////////////////////////////
        else if( strncmp(input[0], "\n", 1) == 0){ continue;}
        ////////////////////////////////////////
        /// user did not enter a valid command//
        ////////////////////////////////////////
        else{printf("please re-try your command\n");}
    }
    exit( 0);
}