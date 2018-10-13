//
// Created by arrjai on 10/5/18.
//

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

//TODO: write doxygen documentation for the fat1 struct
typedef struct {
    unsigned short *entries;
} fat_struct;

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
int read_fat(int fd, boot_struct *bs_pt ,fat_struct *fat_pt);

//functions called by user commands
void fn_help();
int fn_fmount(int * fd, char * fname);
int fn_umount(int fd, char *fname);
int fn_structure(boot_struct *bs_pt);
int fn_showsector(int fd, long sector_num, boot_struct *boot_pt);
int fn_showfat(int fd, fat_struct *fat_pt);

//utility functions
int read_two_byte_hex_num(int fd);

#endif
//PROJ_3_FLOPPY_H
