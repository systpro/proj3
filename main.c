#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <zconf.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "floppy.h"
/**
 * @brief Data from boot sector of floppy organized into a struct.
 *
 * By referring to this stuct, the program has access physical properties of the disk without reading them
 * directly from the floppy image.
 */
boot_struct *boot;

//functions called by user commands

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

//show the content of the specified sector number. This function reads directly from image.
int fn_showsector(int fd, long sector_num)
{
    //print header
    printf("\t\t");
    for(unsigned char header = 0x0; header <= 0xF; header++){
        printf("%2X\t", header);
        if(header == 0xF){
            printf("\n\n");
        }
    }

    //save position of file offset
    off_t temp = lseek(fd, 0, SEEK_CUR);

    //set new position of file offset
    unsigned int byte_offset = (unsigned int) boot->num_bytes_per_sector;
    lseek(fd, sector_num * byte_offset, SEEK_SET);

    //read and print the raw sector data
    unsigned char buf[512];
    unsigned char sidebar = 0x0;
    for(int i = 0; i < 512; i++){
        read(fd, &buf[i], 1 );
        //print sidebar
        if( i % 16 == 0){
            printf("%X0\t\t", sidebar++);
        }
        //
        printf("%2X\t", buf[i]);
        if(((i+1) % 16 == 0) && (i>1)){
            printf("\n");
        }
    }

    //reposition file offset to original location
    lseek(fd, temp, SEEK_SET);
    //TODO: clear warning : control reaches end of non-void function
}

//prints the structure of the floppy disk image
int fn_structure(boot_struct *bs_pt)
{
    printf("number of FAT:\t%21d\n",                bs_pt->num_fat);
    printf("number of sectors used by FAT:%7d\n", bs_pt->num_sectors_fat);
    printf("number of sectors per cluster:%7d\n", bs_pt->num_sectors_per_cluster);
    printf("number of ROOT entries:%14d\n",       bs_pt->num_root_entries);
    printf("number of bytes per sector:%10d\n",    bs_pt->num_bytes_per_sector);

    int nsfat = bs_pt->num_sectors_fat;
    int bs = bs_pt->boot_sector;
    int nre = bs_pt->num_root_entries;
    int bps = bs_pt->num_bytes_per_sector;

    printf("---Sector #---\t---Sector Types---\n");
    printf("\t%d\t%18s\n", bs, "BOOT");
    printf("%d\t--\t%d\t%14s\n",(bs+1), (bs+nsfat), "FAT1");
    printf("%d\t--\t%d\t%14s\n", (bs+nsfat+1), (bs+2*nsfat), "FAT2");
    printf("%d\t--\t%d\t%14s\n", (bs+2*nsfat+1),(bs+2*nsfat)+(nre*32/bps), "ROOT");
    //TODO: clear warning : control reaches end of non-void function
}

//functions that interact with image file
int read_two_byte_hex_num(int fd)
{
    int result = 0;
    unsigned char buf[4];
    //read bytes from image
    for(int i = 0; i <= 3; i += 2){
        read(fd, &buf[i], 1);
    }
    //rearrange two byte numbers to reflect little endianness
    for(int i = 0; i<= 2; i+=2 ) {
        buf[i+1] = buf[i] >> 4;
        buf[i] = buf[i] & (char) 0x0F;
    }
    //convert hex value to decimal
    for(int i = 0; i<=3; i++) {
        double addme = pow(16, (double) i);
        result += buf[i] * (int) addme;
    }
    return result;
}

int read_boot(int fd, boot_struct *bs_pt)
{
    bs_pt->boot_sector = (int) lseek(fd, 0, SEEK_CUR);

    //num_fat
    lseek(fd, 16, SEEK_SET);
    read(fd, &bs_pt->num_fat, 1);

    //num_sectors_fat
    lseek(fd, 22, SEEK_SET);
    bs_pt->num_sectors_fat = read_two_byte_hex_num(fd);

    //num_sectors_per_cluster
    lseek(fd, 13, SEEK_SET);
    read(fd, &bs_pt->num_sectors_per_cluster, 1);

    //num_root_entries
    lseek(fd, 17, SEEK_SET);
    bs_pt->num_root_entries = read_two_byte_hex_num(fd);

    //num_bytes_per_sector
    lseek(fd, 11, SEEK_SET);
    bs_pt->num_bytes_per_sector = read_two_byte_hex_num(fd);

    //num_sectors_filesystem
    lseek(fd, 19, SEEK_SET);
    bs_pt->num_sectors_filesystem = read_two_byte_hex_num(fd);

    //position file offset to end of sector zero
    lseek(fd, 1 * (bs_pt->num_bytes_per_sector), SEEK_SET);
    //TODO: clear warning : control reaches end of non-void function
}

int main()
{
    //the file descriptor for floppy image file
    //TODO: The filename should not be hardcoded. Fname should be set when fmount is called.
    //for debugging purposes you can set the absolute pathname of the .img file below
    char *fname = "../imagefile.img";
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
            //allocate heap storage for structs that hold organized image data.
            //TODO: allocate memory for FAT 1,FAT 2, and root structs.
            boot = (boot_struct*) malloc(sizeof(boot_struct));
            //read data from image file and copy into structs.
            //TODO: perform read and copy operations on FAT 1, FAT 2, and root.
            read_boot(fd, boot);
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
        //display structure of the floppy disk
        else if(strncmp(input[0], structure, 9) == 0){
            fn_structure(boot);
        }
        //print a hex dump of a given sector of the floppy
        else if(strncmp(input[0], showsector, 10) == 0){
            char *next;
            long sector = strtol(input[1], &next, 10);
            fn_showsector(fd, sector);
        }
        //quit program
        else if( strncmp(input[0], quit, 4) == 0){
            break;
        }
        else{printf("please re-try your command\n");}
    }
    free(boot);
    close(fd);
    return 0;
}