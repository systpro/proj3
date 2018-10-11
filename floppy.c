//
// Created by arcticlemming on 10/9/18.
//

#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <zconf.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "floppy.h"

///////////////////////////////////////////////////////////////
///////////////// functions called by user commands ///////////
///////////////////////////////////////////////////////////////

//description of function
void fn_help()
{
    printf("placeholder text\n");
}

//description of function
int fn_fmount(int * fd, char * fname)
{
    *fd = open(fname, O_RDONLY);
    return 0;
}

//description of function
int fn_umount(int fd, char *fname)
{
    close(fd);
    memset(fname, 0, sizeof(fname));
    return 0;
}

//show the content of the specified sector number.
int fn_showsector(int fd, long sector_num, boot_struct *boot_pt)
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
    unsigned int byte_offset = (unsigned int) boot_pt->num_bytes_per_sector;
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
    //@rj-pe TODO: clear warning : control reaches end of non-void function
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
    //@rj-pe TODO: clear warning : control reaches end of non-void function
}

///////////////////////////////////////////////////////////////
/////////////// functions that interact with image file ///////
///////////////////////////////////////////////////////////////
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
    //@rj-pe TODO: clear warning : control reaches end of non-void function
}
