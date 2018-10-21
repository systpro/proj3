
#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <zconf.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "floppy.h"

const unsigned char MSK_RD_ONLY = 0x01;
const unsigned char MSK_HIDDEN = 0x02;
const unsigned char MSK_SYSTEM = 0x04;
const unsigned char MSK_ARCH = 0x20;
const unsigned char MSK_VOLUME = 0x08;
const unsigned char MSK_SUBDIR = 0x10;
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
    //TODO: read all 512 with a single system call
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

int fn_showfat(fat_struct **fat_pt)
{

    // print header
    printf("   ");
    for(unsigned char header = 0x0; header <= 0xF; header++){
        printf("  %X  ", header);
        if(header == 0xF){
            printf("\n\n");
        }
    }
    unsigned char sidebar = 0x0;

    for(int i = 0; i < 256; i++){
        //print sidebar
        if( i % 16 == 0){
            printf("%X0  ", sidebar++);
        }
        //don't print the first two fat1 table entries
        if(i == 0 || i == 1){
            printf("     ");
            continue;
        }
        //if the entry contains a 0 then print "FREE"
        if(fat_pt[i]->address == 0){
            printf("FREE ");
        } else {
            printf("%4x ", fat_pt[i]->address);
        }
        //print a newline at the end of each row
        if(((i+1) % 16 == 0) && (i>1)){
            printf("\n");
        }
    }
    return 0;
}

int fn_traverse(root_struct **root_pt, int entries, fat_struct **fat_pt, int fd)
{
    //TODO: sort files and directories alphabetically before printing
    //TODO: figure out how nested directories work

    for(int i = 0; i < entries; i++ ){
        if( (! check_mask(root_pt[i]->attribute, MSK_HIDDEN)) && (! root_pt[i]->available) ) {

            if( check_mask(root_pt[i]->attribute, MSK_SUBDIR)){
                int rename_this_variable = check_dir_contents(fat_pt, root_pt[i]->first_logical_cluster, fd);

                printf("%s\t<DIR>\n", root_pt[i]->filename);
            } else {

                printf("%s\n", root_pt[i]->filename);
            }
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

//prints a hex dump of a given file return -1 if file not found
int fn_showfile(file_struct **files, int file_count, char *filename)
{
    //loop through filenames searching for a match
    int i = 0;
    while(i < file_count){
        if(strcmp(files[i]->filename, filename) == 0){
            break;
        } else {
            i++;
        }
    }
    //TODO : should i be greater than?
    if(i == file_count){
        return -1;
    } else{
        //TODO: print header row
        int sidebar = 0x0;
        for(int g = 0; g < files[i]->length; g++ ){
            //print sidebar
            if( g % 16 == 0){
                printf("%X0\t\t", sidebar++);
            }
            //print data
            printf("%X\t", files[i]->data[g]);
            //print newline
            if(((g+1) % 16 == 0) && ( g > 1)){
                printf("\n");
            }
        }
        return i;
    }
}

///////////////////////////////////////////////////////////////
/////////////// utility functions /////////////////////////////
///////////////////////////////////////////////////////////////

int allocate_file(file_struct *file, fat_struct **fat_pt, ushort first_cluster, int cluster_bytes)
{
    //allocate cluster list
    int clusters = 0;
    //look ahead to obtain the cluster list allocation size
    for(ushort i = first_cluster; i < 0xFF0; i = fat_pt[(int)i]->address) {
        clusters ++;
    }
    //allocate memory for cluster list
    file->cluster_list = (list *) calloc((size_t) clusters, sizeof(list ));
    //allocate memory for file data
    file->data = (unsigned char *) calloc((size_t) clusters * cluster_bytes, sizeof(unsigned char));
    return clusters;
}

int read_file_data(int fd, int cluster_bytes, file_struct *file)
{
    if(file->length == 0){
        return 0;
    }
    int clusters = 0;
    //loop through the cluster addresses in cluster list
    for(off_t i = file->cluster_list[clusters].address; clusters < file->cluster_count; ++clusters){
        //set the file offset to cluster address
        lseek(fd, i * cluster_bytes, SEEK_SET);
        //read contents of cluster into buffer
        read(fd, &file->data[clusters * cluster_bytes], (size_t) cluster_bytes);
        i = file->cluster_list[clusters].next;
    }
    return clusters;
}

off_t create_address(off_t start, ushort convert)
{
    return start + (off_t) convert - 2;
}

int build_cluster_list(off_t data_start, file_struct *file, ushort first, fat_struct **fat_pt)
{
    //for every fat entry in chain add corresponding data to cluster list
    int clusters = file->cluster_count;
    ushort i = first;
    for(ushort a = 0; i < 0xFF0; i = fat_pt[i]->address, a++){
        file->cluster_list[a].address = create_address(data_start, i);
        if(a < clusters - 1) {
            file->cluster_list[a].next = create_address(data_start, fat_pt[i]->address);
        } //the last cluster in a file has a null value in next
        else if(a == clusters - 1){
            file->cluster_list[a].next = 0;
        }
    }
    return 0;
}

int get_file_count(root_struct **rt_pt, int entries)
{
    int file_count = 0;
    ///loop through root dir and save any entries that contain a valid file return count of valid files
    for(int i = 0; i < entries; i++){
        if(! rt_pt[i]->available && (rt_pt[i]->first_logical_cluster > 1)){
            file_count ++;
        }
    }
    return file_count;
}

int check_dir_contents(fat_struct **fat_pt, unsigned short first_cluster, int fd)
{
    int entries = 0;
    //hardcoding of 33 is not optimal. Replace with call to the proper variables from a boot_sector pt
    off_t address = 33 + first_cluster - 2;
    lseek(fd, address, SEEK_SET);
    fat_pt[first_cluster];
    return entries;
}

void print_dir(root_struct **root_pt, int entries)
{
    for(int i =0; i < entries; i++){
        if(check_mask( root_pt[i]->attribute, MSK_SUBDIR)){
            printf("%s%d\n",
                   root_pt[i]->filename,
                   root_pt[i]->first_logical_cluster);
        }
    }
}

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
        buf[i+1] = buf[i] >> 4u;
        buf[i] = buf[i] & (unsigned char) 0x0Fu;
    }
    //convert hex value to decimal
    for(int i = 0; i<=3; i++) {
        double addme = pow(16, (double) i);
        result += buf[i] * (int) addme;
    }
    return result;
}

unsigned long read_ulong(const unsigned char *bytes, size_t pos)
{
    unsigned long result = 0;
    size_t zero = pos;
    size_t one = pos + 1;
    size_t two = pos + 2;
    size_t three = pos + 3;
    unsigned long byte1 = (unsigned long) bytes[three] << 24u;
    unsigned long byte2 = (unsigned long) bytes[two] << 16u;
    unsigned long byte3 = (unsigned long) bytes[one] << 8u;
    unsigned long byte4 = (unsigned long) bytes[zero];
    result = byte1 | byte2 | byte3 | byte4 ;
    return result;
}

unsigned short read_ushort(const unsigned char *bytes, size_t pos)
{
    unsigned short result = 0;
    size_t zero = pos;
    size_t one = pos + 1;
    unsigned short byte1 = (unsigned short) bytes[one] << 8u;
    unsigned short byte2 = (unsigned short) bytes[zero];
    result = byte1 | byte2 ;
    return result;
}

int check_mask(unsigned char att_byte, unsigned char mask)
{
    return (att_byte & mask);
}
//given the attribute field (stored in the root_struct entries)
// this function returns a print ready string
void get_attributes(unsigned char att_byte, char *string)
{
    if(check_mask(att_byte, MSK_RD_ONLY) == 1){
        string[1] = 'R';
    }
    if(check_mask(att_byte, MSK_SYSTEM) == 1){
        string[2] = 'S';
    }
    if(check_mask(att_byte, MSK_HIDDEN) == 1){
        string[3] = 'H';
    }
    if(check_mask(att_byte, MSK_ARCH) == 1){
        string[4] = 'A';
    }
}

int create_date(struct tm *date, unsigned char *bytes, size_t pos)
{
    size_t zero = pos;
    size_t one = pos + 1;
    //map year, month, day from bytes to struct
    date->tm_year = (bytes[one] >> 1u) + 80;
    date->tm_mon = ((unsigned short)((bytes[one] & 0x01u) << 3u) | (unsigned short)(bytes[zero] & 0xD0u >> 5u) ) - 1;
    date->tm_mday = (unsigned short) (bytes[zero] & 0x1Fu);
    return 0;
}

int create_time(struct tm *time, unsigned char *bytes, size_t pos)
{
    size_t zero = pos;
    size_t one = pos + 1;
    time->tm_hour = bytes[one] >> 3u;
    time->tm_min = ((unsigned short)((bytes[one] & 0x03u) << 3u) | (unsigned short)(bytes[zero] & 0xD0u >> 5u) );
    time->tm_sec = (unsigned short) (bytes[zero] & 0x1Fu) * 2;
    //map seconds, minutes, hours from bytes to struct
    return 0;
}


///////////////////////////////////////////////////////////////
/////////////// functions that interact with image file ///////
///////////////////////////////////////////////////////////////

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

int read_fat(int fd, boot_struct *bs_pt ,fat_struct **fat_pt)
{
    int nsf = bs_pt->num_sectors_fat;
    int bps = bs_pt->num_bytes_per_sector;
    int fat1_size = bps * bs_pt->num_reserved_sectors;
    unsigned char raw_fat[nsf*bps];
    unsigned char buf[3];

    //read raw fat1_size data from image into a buffer
    lseek(fd, fat1_size, SEEK_SET);

    //read raw fat1_size data from image into a buffer
    lseek(fd, fat1_size, SEEK_SET);
    read(fd, raw_fat, (size_t) nsf*bps);

    for(int i = 0; i < (nsf*bps/6)-1; i++){
        //grab three bytes from raw fat1_size data buffer
        buf[0] = raw_fat[i*3];
        buf[1] = raw_fat[i*3+1];
        buf[2] = raw_fat[i*3+2];
        /*parse three bytes into two fat1_size entries*/
        fat_pt[i*2]->address =
                (((unsigned short) buf[1] & 0x0Fu) << 8u ) |
                ((unsigned short) buf[0]);
        fat_pt[i*2+1]->address= (((unsigned short) buf[2]) << 4u)|(((unsigned short) buf[1]) >> 4u);
    }
    return 0;
}

int read_root(int fd, boot_struct *bs_pt, root_struct **rt_pt)
{
    int nf = bs_pt->num_fat;
    int bps = bs_pt->num_bytes_per_sector;
    int nsf = bs_pt->num_sectors_fat;
    int nrs = bs_pt->num_reserved_sectors;
    int nre = bs_pt->num_root_entries;
    int root_pos = nf * bps * nsf + nrs * bps;
    //TODO: is the size of a dir. entry constant across all custom FAT configs?
    int sde = 32;
    unsigned char raw_root[nre * sde];
    unsigned char buf[32];

    //read raw root data from image into a buffer
    lseek(fd, root_pos, SEEK_SET);
    read(fd, raw_root, (size_t) nre*sde);
    //loop through raw data 32 bytes at a time
    for(int i = 0; i < nre; i++) {
        //for ease of access temporarily store the next 32 bytes
        for (int j = i*32, k = 0; k < 32; j++, k++) {
            buf[k] = raw_root[j];
        }
        //check for special values in first byte
        if(buf[0] == 0x00){
            rt_pt[i]->available = 1;
            continue;
        }
        rt_pt[i]->available = 0;
        //extract file size from raw data.
        rt_pt[i]->file_size = read_ulong(&buf[0], 28);
        //extract attributes from raw data.
        rt_pt[i]->attribute = buf[11];
        if((rt_pt[i]->file_size == 0) && ! check_mask(rt_pt[i]->attribute, MSK_SUBDIR)){
            continue;
        }
        if(check_mask(rt_pt[i]->attribute, MSK_HIDDEN)) {
            continue;
        }

        //extract filename from raw data bytes 0-8
        strncpy(rt_pt[i]->filename, (char *) &buf[0], 8);
        //extract extension from raw data
        strncpy(rt_pt[i]->extension, (char *) &buf[8], 3);
        if((rt_pt[i]->extension[0] != 0) && (! check_mask(rt_pt[i]->attribute, MSK_SUBDIR))) {
            if(strchr(rt_pt[i]->filename, 32) != NULL){
            strncpy(strchr(rt_pt[i]->filename, 32), ".", 1);
            } else{
                strncpy(strchr(rt_pt[i]->filename, 0), ".", 1);
            }
            strncpy(strchr(rt_pt[i]->filename, '.') + 1, ("%s\0", rt_pt[i]->extension), 4);
        }
        memmove(rt_pt[i]->filename + 1, rt_pt[i]->filename, strlen(rt_pt[i]->filename) + 1);
        rt_pt[i]->filename[0] = '/';
        //extract creation time from raw data.
        create_time(&rt_pt[i]->creation_time, &buf[0], 14);
        //extract creation date from raw data.
        create_date(&rt_pt[i]->creation_date, &buf[0], 16);
        //extract last access date from raw data.
        create_date(&rt_pt[i]->last_access_date, &buf[0], 18);
        //extract last write time from raw data.
        create_time(&rt_pt[i]->last_write_time, &buf[0], 22);
        //extract last write date from raw data.
        create_date(&rt_pt[i]->last_write_date, &buf[0], 24);
        //extract first logical cluster from raw data.
        rt_pt[i]->first_logical_cluster = read_ushort(&buf[0], 26);
    }
    return 0;
}

int read_files(int fd, int entries,
               off_t data_start, int cluster_bytes,
               file_struct **f_pt, root_struct **rt_pt,
               fat_struct **fat_pt)
{
    int entry = 0;
    //loop through root & copy required fields
    for(int i = 0; i < entries; i++){
        if((! rt_pt[i]->available) && (rt_pt[i]->first_logical_cluster > 1)){
            f_pt[entry]->cluster_count =
                    allocate_file(f_pt[entry], fat_pt, rt_pt[i]->first_logical_cluster, cluster_bytes);
            f_pt[entry]->length = f_pt[entry]->cluster_count * cluster_bytes;
            strcpy(f_pt[entry]->filename, rt_pt[i]->filename);
            //starting at the first cluster build the files cluster list
            build_cluster_list(data_start, f_pt[entry], rt_pt[i]->first_logical_cluster, fat_pt);
            //read the file data from image data sectors
            if(f_pt[entry]->cluster_count == read_file_data(fd, cluster_bytes, f_pt[entry])){
                entry++;
                continue;
            } else{
                printf("error reading file %s", f_pt[entry]->filename);
            }
        }
    }
    return entry;
}
