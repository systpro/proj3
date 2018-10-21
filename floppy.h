//
// Created by arrjai on 10/5/18.
//
#include <time.h>
#ifndef PROJ_3_FLOPPY_H
#define PROJ_3_FLOPPY_H

//structs that hold data extracted from image file
/**
 * \struct boot_struct
 * \brief structure that holds data taken from sector zero of the floppy image.
 * \var boot_struct,boot_sector
 * contains the location of the boot sector.
 * \var boot_struct.num_fat
 * contains the number of FATs in image. Starting byte: 16, length (in bytes): 1.
 * \var boot_struct.num_sectors_fat
 * contains the number of sectors that a single allocation table occupies. Starting byte: 22, length (in bytes): 2.
 * \var boot_struct.num_sectors_per_cluster
 * contains the number of sectors that each cluster occupies. Starting byte: 13, length (in bytes): 1.
 * \var boot_struct.num_root_entries
 * contains the number of root entries the floppy image has. Starting byte: 17, length (in bytes): 2.
 * \var boot_struct.num_bytes_per_sector
 * contains the number of bytes that each sector occupies. Starting byte: 11, length (in bytes): 2.
 * \var boot_struct,num_sectors_filesystem
 * contains the total number of sectors in the filesystem Starting byte: 19, length (in bytes): 2.
 * \var boot_struct.num_reserved_sectors
 * contains the number of reserved sectors. Starting byte: 14, length(in bytes): 2.
 */
typedef struct {
    int boot_sector;
    int num_fat;
    int num_sectors_fat;
    int num_sectors_per_cluster;
    int num_root_entries;
    int num_bytes_per_sector;
    int num_sectors_filesystem;
    int num_reserved_sectors;
} boot_struct;

//TODO: write doxygen documentation for the fat struct
typedef struct {
    unsigned short address;
} fat_struct;

//TODO: write doxygen documentation for the root struct
typedef struct{
    char filename[13];
    char extension[4];
    unsigned char attribute;
    struct tm creation_time;
    struct tm creation_date;
    struct tm last_access_date;
    struct tm last_write_time;
    struct tm last_write_date;
    unsigned short first_logical_cluster;
    unsigned long file_size;
    char available;
} root_struct;

typedef struct{
    off_t address;
    off_t next;
} list;

//TODO: write doxygen documentation for file_struct
typedef struct{
    char filename[13];
    int cluster_count;
    int length;
    unsigned char *data;
    list *cluster_list;
} file_struct;

//functions that interact with image file
/**
 * \fn read_boot
 * function reads the zeroth sector of a floppy image and stores physical information in a struct.
 * \param fd the file descriptor, assigned to the floppy image by mount.
 * \param bs_pt the address of the structure variable that will hold data that is read from image file.
 * \return  the final position of the file offset pointer. A successful read_boot call leaves the file offset pointer
 * at the beginning of sector one.
 */
int read_boot(int fd, boot_struct *bs_pt);

//TODO: write doxygen documentation for read_fat
int read_fat(int fd, boot_struct *bs_pt, fat_struct **fat_pt);

//TODO: write documentation for read_root
int read_root(int fd, boot_struct *bs_pt, root_struct **rt_pt);

//TODO: write documentation for read_file
int read_files(int fd, int entries, off_t data_start, int cluster_bytes, file_struct **f_pt, root_struct **rt_pt, fat_struct **fat_pt);

//functions called by user commands
void fn_help();
int fn_fmount(int * fd, char * fname);
int fn_umount(int fd, char *fname);
int fn_structure(boot_struct *bs_pt);
int fn_showsector(int fd, long sector_num, boot_struct *boot_pt);
int fn_showfat(fat_struct **fat_pt);
int fn_traverse(root_struct **root_pt, int entries, fat_struct **fat_pt, int fd);
int fn_showfile(file_struct **f_pt, int file_count, char *filename);

//utility functions
int read_two_byte_hex_num(int fd);
unsigned long read_ulong(const unsigned char *bytes, size_t pos);
unsigned short read_ushort(const unsigned char *bytes, size_t pos);
void get_attributes(unsigned char att_byte, char *string);
int check_mask(unsigned char att_byte, unsigned char mask);
int create_date(struct tm *date, unsigned char *bytes, size_t position);
int create_time(struct tm *time, unsigned char *bytes, size_t position);
void print_dir(root_struct **root_pt, int entries);
//looks through the sector data for contents of a given directory. Returns number of items found.
int check_dir_contents(fat_struct **fat_pt, unsigned short fat_cluster, int fd);
//returns a count of root entries not listed as free
int get_file_count(root_struct **root_pt, int entries);
//given a fat table index and a file struct entry this function builds the files cluster list
//returns the number of cluster's that a file occupies
int build_cluster_list(off_t data_start, file_struct *file, unsigned short first, fat_struct **fat_pt);
//given a fat entry returns the corresponding address w.r.t start of image
off_t create_address(off_t data_start, ushort entry);
//given a file struct read the file contents from image into struct
//returns the number of clusters read
int read_file_data(int fd, int cluster_bytes, file_struct *file);
int allocate_file(file_struct *file, fat_struct **fat_pt, ushort first_cluster, int cluster_bytes);

//constants
extern const unsigned char MSK_RD_ONLY;
extern const unsigned char MSK_HIDDEN;
extern const unsigned char MSK_SYSTEM;
extern const unsigned char MSK_ARCH;
extern const unsigned char MSK_VOLUME;
extern const unsigned char MSK_SUBDIR;

#endif
//PROJ_3_FLOPPY_H
