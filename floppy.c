
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
    if(*fd < 0 ){
        printf("%s\n", strerror(errno));
    }
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
    //@rj-pe TODO: remove hardcoding of 512 and replace with num_bytes_per_sector
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

int fn_showfat(int fd, fat_struct *fat_pt){

    /* TODO: Format this table properly. The correct information is displayed, but formatting is bad.
    */
    // print header
    printf("\t\t");
    for(unsigned char header = 0x0; header <= 0xF; header++){
        printf("%2X\t", header);
        if(header == 0xF){
            printf("\n\n");
        }
    }
    unsigned char sidebar = 0x0;

    for(int i = 0; i < 256; i++){
        //print sidebar
        if( i % 16 == 0){
            printf("%X0\t\t", sidebar++);
        }
        //don't print the first two fat1 table entries
        if(i == 0 || i == 1){
            printf("\t");
            continue;
        }
        //if the entry contains a 0 then print "FREE"
        if(fat_pt->entries[i] == 0){
            printf("FREE ");
        } else {
            printf("%2x\t", fat_pt->entries[i]);
        }
        //print a newline at the end of each row
        if(((i+1) % 16 == 0) && (i>1)){
            printf("\n");
        }
    }
    return 0;
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

    //num_reserved_sectors
    lseek(fd, 14, SEEK_SET);
    bs_pt->num_reserved_sectors = read_two_byte_hex_num(fd);

    //position file offset to end of sector zero
    lseek(fd, 1 * (bs_pt->num_bytes_per_sector), SEEK_SET);
    return 0;
}

int read_fat(int fd, boot_struct *bs_pt ,fat_struct *fat_pt){
    int nsf = bs_pt->num_sectors_fat;
    int bps = bs_pt->num_bytes_per_sector;
    int fat1 = bps * bs_pt->num_reserved_sectors;
    unsigned char raw_fat[nsf*bps];
    unsigned char temp_buf[3];

    //read raw fat1 data from image into a buffer
    lseek(fd, fat1, SEEK_SET);
    read(fd, raw_fat, (size_t) nsf*bps);

    fat_pt->entries = (unsigned short *) realloc(fat_pt->entries, nsf * bps / 3 * sizeof(unsigned short));
    memset(fat_pt->entries, 0, sizeof(unsigned short*));

    for(int i = 0; i < nsf*bps/3; i++){
        //grab three bytes from raw fat1 data buffer
        temp_buf[0] = raw_fat[i*3];
        temp_buf[1] = raw_fat[i*3+1];
        temp_buf[2] = raw_fat[i*3+2];
        /*parse three bytes into two fat1 entries*/
        fat_pt->entries[i*2]= (((unsigned short) temp_buf[1] & 0x0F) << 8 ) | ((unsigned short) temp_buf[0]);
        fat_pt->entries[i*2+1]= (((unsigned short) temp_buf[2]) << 4)|(((unsigned short) temp_buf[1]) >>4);
    }
    return 0;
}