#define _FILE_OFFSET_BITS 64
#define SECTOR_LENGTH 512

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <limits.h>
#include "fat32.h"
#include "fat_fs.h"


#define CMD_INFO "info"
#define CMD_LIST "list"
#define CMD_GET "get"
#define FILE_NAME_LENGTH 8
#define EXTENSION_LENGTH 3

#define DIRECTORY_ENTRY_FREE 0xE5
#define ELEVEN 11
#define MASKBY 16
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

int fd;
char *file_name;
char *pathToFile;
bool fileFound = false;
char buffer[SECTOR_LENGTH];
char *path;

// global var for equations
uint32_t total_root_ent_cnt;
uint32_t byte_per_sec_off_by_1;
uint32_t root_dir_sectors;
uint32_t first_data_sector;
uint32_t total_Dir_Number;

fat32BS *boot_sector;
Dir_Info *curr_dir;
FS_Info *fs_info;

uint32_t rootCluster;

int main(int argc, char *argv[])
{
    //VolumeName = (char *)malloc(sizeof(char) * 100);
    
    file_name = argv[1];
    
    // allocate space for the boot sector
    boot_sector = (fat32BS *)malloc(sizeof(fat32BS));
    
    // allocate space for the structures
    fs_info = malloc(sizeof(FS_Info));
    curr_dir = malloc(sizeof(Dir_Info));
    
    if (argc < 3) {
        printf("Usage: compiled file name <image name> <Command to execute>");
        exit(1);
    
    } else {
        
        fd = open(file_name, O_RDONLY);
        
        // read the boot sector
        read(fd, buffer, sizeof(fat32BS));
        memcpy(boot_sector, buffer, sizeof(fat32BS));
        
        // get the root sector
        rootCluster = boot_sector->BPB_RootClus;
        
        // read the file system information
        read(fd, buffer, sizeof(FS_Info));
        memcpy(fs_info, buffer, sizeof(FS_Info));
        
        
        // constant global values
        total_root_ent_cnt = boot_sector->BPB_RootEntCnt * 32;
        byte_per_sec_off_by_1 = boot_sector->BPB_BytesPerSec - 1;
        root_dir_sectors = (total_root_ent_cnt + byte_per_sec_off_by_1) / boot_sector->BPB_BytesPerSec;
        first_data_sector = boot_sector->BPB_RsvdSecCnt + (boot_sector->BPB_NumFATs * boot_sector->BPB_FATSz32) + root_dir_sectors;
        total_Dir_Number = (boot_sector->BPB_SecPerClus * boot_sector->BPB_BytesPerSec) / sizeof(Dir_Info);
        
        // get the argument
        char *command_to_execute = argv[2];
        switch (argc) {
            case 3:
                if (strcmp(command_to_execute, CMD_INFO) == 0) {
                    print_info(boot_sector);
                } else if (strcmp(command_to_execute, CMD_LIST) == 0) {
                    
                    // print_root info
                    uint32_t first_data_sector = get_sector_of_cluster(boot_sector, rootCluster);
                    uint32_t dir_offset = first_data_sector * boot_sector->BPB_BytesPerSec;
                    
                    // seek to the location and read it into the buffer
                    lseek(fd, dir_offset, SEEK_SET);
                    read(fd, buffer, sizeof(Dir_Info));
                    memcpy(curr_dir, buffer, sizeof(Dir_Info));
                    
                    printf("Root Directory: %.*s\n", ELEVEN, curr_dir->DIR_Name);
                    
                    // print the inner directories
                    print_dir_recursively(boot_sector, rootCluster, 0);
                }
                break;
                
            case 4:
                if (strcmp(command_to_execute, CMD_GET) == 0) {
                    
                    // global var for path
                    path = argv[3];
                    
                    if (path != NULL && strlen(path) > 0) {
                        getFileContent(boot_sector, rootCluster);
                        if(fileFound == false) {
                            printf("%s\n","File not found");
                        }
                    } // if
                } else {
                    printf("%s\n","Invalid command");
                }
                break;
            default:
                printf("Usage: compiled file name <image name> <info OR list OR get path/to/file>");
                exit(1);
        }
    }
    
    // clean up resources
    free(boot_sector);
    free(curr_dir);
    free(fs_info);
    printf("%s\n", "Program ended successfully");
    return 0;
} // close main


void print_contents_of_the_file(fat32BS *bs, uint32_t clus_num, char *file_name) {
    uint32_t first_sector_of_file = get_sector_of_cluster(bs, clus_num);
    uint32_t file_offset = first_sector_of_file * bs->BPB_BytesPerSec;
    uint32_t clus_size = bs->BPB_SecPerClus * bs->BPB_BytesPerSec;
    
    char *file_buffer = seek_and_read_file_contents(clus_size, file_offset);
    
    if (file_buffer != NULL && strlen(file_buffer) > 0) {
        printf("Started reading contents of the file: %s\n", file_name);
        printf("--------------\n");
        
        int i = 0;
        for (; file_buffer[i] != 0x00; i++) {
            printf("%c", file_buffer[i]);
        } // for
        printf("\n--------------\n");
        printf("%s\n", "Finished reading contents of the file");
    }
    
} // close print_contents_of_the_file


char *seek_and_read_file_contents(uint32_t clus_size, uint32_t file_offset) {
    char *file_buffer = malloc(sizeof(char) * clus_size);
    lseek(fd, file_offset, SEEK_SET);
    read(fd, file_buffer, clus_size);
    return file_buffer;
} // close seek_and_read_file_contents


uint32_t get_dir_offset(fat32BS *bs, uint32_t cluster_num) {
    uint32_t first_data_sector = get_sector_of_cluster(bs, cluster_num);
    uint32_t dir_offset = first_data_sector * bs->BPB_BytesPerSec;
    return dir_offset;
} // close get_dir_offset


void seek(int i, uint32_t dir_offset) {
    // seek to the location and read the data into a buffer
    lseek(fd, dir_offset + (i * sizeof(Dir_Info)), SEEK_SET);
    read(fd, buffer, sizeof(Dir_Info));
    memcpy(curr_dir, buffer, sizeof(Dir_Info));
} // close seek


void name_parser(char *name) {
    name = strtok(name, " ");
    // terminate with null byte
    name[strlen(name)] = '\0';
} // close name_parser


int print_dir_recursively(fat32BS *bs, uint32_t cluster_num, uint32_t indent) {
    
    uint32_t dir_offset = get_dir_offset(bs, cluster_num);

    int i;
    for (i = 0; i < total_Dir_Number; i++) {
        
        seek(i, dir_offset);
        
        // variables for checking dir name + attribute
        char *dir_name_check = curr_dir->DIR_Name[0];
        char *curr_dir_attr = curr_dir->DIR_Attr;
        
        bool result = is_directory(curr_dir);
        if (result) {
            if (curr_dir_attr == MASKBY && (dir_name_check != DIRECTORY_ENTRY_FREE)) {
                
                print_dash(indent);
                char *dir_name = curr_dir->DIR_Name;
                printf("Directory Name: %.*s\n", ELEVEN, dir_name);
                
                // mask the bits and get the next cluster
                uint32_t next_cluster_number_dir = curr_dir->DIR_FstClusHI << MASKBY | curr_dir->DIR_FstClusLO;
                
                // call the recursive function
                print_dir_recursively(bs, next_cluster_number_dir, indent++);
                
            } else if (dir_name_check != DIRECTORY_ENTRY_FREE) {
                char *name = malloc(sizeof(8));
                char *ext = malloc(sizeof(3));
                
                char *dir_name = curr_dir->DIR_Name;
                
                // copy the name + the extension
                strncpy(name, dir_name, 8);
                strncpy(ext, dir_name + 8, 3);
                
                // tokenize the name
                name_parser(name);
                
                print_dash(indent);
                printf("File: %s.%s\n", name, ext);
            }
        }
    }
    
    // check if we have a cluster chain
    uint32_t next_cluster = check_chain(bs, cluster_num);
    
    // mask the first 4 bits
    next_cluster = next_cluster & 0xfffffff;
    
    if (next_cluster != 0xffffff8 && next_cluster != 0xfffffff) {
        print_dir_recursively(bs, next_cluster, indent);
    }
    return 0;
} // close print_dir_recursively


bool is_directory(Dir_Info *currDir) {
    bool result = false;
    result = (currDir->DIR_Attr == ATTR_DIRECTORY || currDir->DIR_Attr == ATTR_ARCHIVE) && currDir->DIR_Name[0] != '.' && currDir->DIR_Name[0] != 0xFFFFFFE5;
    return result;
} // close is_directory


uint32_t get_sector_of_cluster(fat32BS *bs, uint32_t n) {
    // Microsoft fat32 white paper equation
    uint32_t data_sector = ((n - 2) * bs->BPB_SecPerClus) + first_data_sector;
    return data_sector;
} // close get_sector_of_cluster


char *tokenize_path() {
    char *tokenize_path_copy = malloc(strlen(path));
    
    char *dir_file_name = "";
    tokenize_path_copy = strdup(path);
    
    dir_file_name = tokenize_dir_file_name(tokenize_path_copy);
    
    if (dir_file_name != NULL && strlen(dir_file_name) > 0) {
        unsigned long path_len = strlen(path);
        unsigned long dir_len = strlen(dir_file_name);
        
        // get the new path after tokenzing
        char *new_path = malloc(sizeof(100));
        memcpy(new_path, path+dir_len+1, path_len-dir_len);
        path = strdup(new_path);
    } // if
    return dir_file_name;
} // close tokenize_path


char *tokenize_dir_file_name(char *tokenize_path_copy) {
    char *dir_file_name = malloc(sizeof(64));
    dir_file_name = strtok(tokenize_path_copy, "/");
    dir_file_name[strlen(dir_file_name)] = '\0';
    return dir_file_name;
} // close tokenize_dir_file_name


uint32_t check_chain(fat32BS *bs, uint32_t cluster_number) {
    uint32_t FATOffset = 0;
    
    // asuming the file type is always fat32
    FATOffset = cluster_number * 4;
    
    // equations from the white paper
    uint32_t ThisFATSecNum = bs->BPB_RsvdSecCnt + (FATOffset / bs->BPB_BytesPerSec);
    uint32_t ThisFATEntOffset = FATOffset % bs->BPB_BytesPerSec;
    
    // store the next cluster
    uint32_t next_cluster;
    
    lseek(fd, (bs->BPB_BytesPerSec * ThisFATSecNum) + ThisFATEntOffset, SEEK_SET);
    // read in four bytes only in FAT
    read(fd, &next_cluster, 4);
    
    return next_cluster;
} // close check_chain


void print_info(fat32BS *bs) {
    
    uint32_t totalSize = (bs->BPB_TotSec32 * bs->BPB_BytesPerSec) / 1024;
    uint32_t totalReserved = ((bs->BPB_RsvdSecCnt + bs->BPB_NumFATs * bs->BPB_FATSz32) * bs->BPB_BytesPerSec) / 1024;
    uint32_t freeSpace = (fs_info->FSI_Free_Count * bs->BPB_SecPerClus * bs->BPB_BytesPerSec) / 1024;
    float clusterSizeInKB = (float)(bs->BPB_SecPerClus * bs->BPB_BytesPerSec) / 1024.00;
    
    
    printf("\nDisk information:\n");
    printf("-----------------\n");
    printf("OEM Name: %.8s\n", bs->BS_OEMName);
    printf("Volume Label: %.11s\n", bs->BS_VolLab);
    printf("File System Type (text): %.8s\n", bs->BS_FilSysType);
    
    printf("\nDisk geometry:\n");
    printf("--------------\n");
    
    printf("Bytes Per Sector: %d\n", bs->BPB_BytesPerSec);
    printf("Sectors Per Cluster: %d\n", bs->BPB_SecPerClus);
    printf("Total Sectors: %d\n", (0 == bs->BPB_TotSec32) ? bs->BPB_TotSec16 : bs->BPB_TotSec32);
    printf("Physical - Sectors per Track: %d\n", bs->BPB_SecPerTrk);
    printf("Physical - Heads: %d\n", bs->BPB_NumHeads);
    
    printf("\nSize info:\n");
    printf("--------------\n");
    printf("Total size: %dKB\n", totalSize);
    printf("Free space on the drive: %uKB\n", freeSpace);
    printf("Usuable space on the drive: %uKB\n", totalSize - totalReserved);
    printf("The cluster size in number of sectors: %d\n", bs->BPB_SecPerClus);
    printf("The cluster size in number of sectors: %3.1fKB\n", clusterSizeInKB);
    printf("--------------\n");
} // close print_info


void print_dash(int indent) {
    int i;
    for (i = 0; i < indent; i++) {
        printf("-");
    } // end for
} // close print_dash


void getFileContent(fat32BS *bpb, uint32_t clusterNum) {
    char *copyPath = (char *)malloc(strlen(path));
    copyPath = strdup(path);
    char * fileName = tokenize_path();
    
    uint32_t firstDataSector = get_sector_of_cluster(bpb, clusterNum);
    uint32_t dir_offset = firstDataSector * bpb->BPB_BytesPerSec;
    
    int i = 0;
    for (; i < total_Dir_Number; i++) {
        seek(i, dir_offset);
        
        uint32_t dir_attr = curr_dir->DIR_Attr;
        
        bool result = is_directory(curr_dir);
        if (result) {
            char *dirName = malloc(sizeof(FILE_NAME_LENGTH));
            strncpy(dirName, curr_dir->DIR_Name, FILE_NAME_LENGTH);
            dirName = strtok(dirName, " ");
            
            if (dir_attr == 16 && (curr_dir->DIR_Name[0] != 0xe5) && strcasecmp(dirName, fileName) == 0) {
                uint32_t clusterNumber_Dir = curr_dir->DIR_FstClusHI << MASKBY | curr_dir->DIR_FstClusLO;
                getFileContent(bpb, clusterNumber_Dir);
            } else if (curr_dir->DIR_Name[0] != 0xe5) {
                char *name = malloc(sizeof(FILE_NAME_LENGTH));
                char *extension = malloc(sizeof(EXTENSION_LENGTH));
                
                strncpy(name, curr_dir->DIR_Name, FILE_NAME_LENGTH);
                strncpy(extension, curr_dir->DIR_Name + FILE_NAME_LENGTH, EXTENSION_LENGTH);
                name = strtok(name, " ");
                char *fileNameToCheck = (char *)malloc(sizeof(FILE_NAME_LENGTH + EXTENSION_LENGTH));
                
                strcat(fileNameToCheck, name);
                strcat(fileNameToCheck, ".");
                strcat(fileNameToCheck, extension);
                if (strcasecmp(fileNameToCheck, fileName) == 0) {
                    
                    uint32_t clusterNumber_File = curr_dir->DIR_FstClusHI << MASKBY | curr_dir->DIR_FstClusLO;;
                    
                    print_contents_of_the_file(bpb, clusterNumber_File, fileNameToCheck);
                    uint32_t next_cluster = check_chain(bpb, clusterNumber_File);
                    
                    // mask the first 4 bits
                    next_cluster = next_cluster & 0xfffffff;
                    while (next_cluster == 0xffffff8 || next_cluster == 0xfffffff) {
                        print_contents_of_the_file(bpb, next_cluster, fileNameToCheck);
                        next_cluster = check_chain(bpb, next_cluster);
                        next_cluster = next_cluster & 0xfffffff;
                    }
                    fileFound = true;
                    return;
                }
            }
        }
    }
    uint32_t next_cluster = check_chain(bpb, clusterNum);
    next_cluster = next_cluster & 0xfffffff;
    if (next_cluster != 0xffffff8 && next_cluster != 0xfffffff) {
        getFileContent(bpb, next_cluster);
    }
} // close getFileContent
