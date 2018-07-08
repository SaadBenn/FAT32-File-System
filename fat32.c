#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "fat_fs.h"
#include "fat32.h"

int main(int argc, char *argv[]) {
	
	FS_Instance *fat32_instance;
	FS_CurrentDir current_dir;
	// check the required arguments
	if (argc != 3) {
		perror("Usage: <./fat32 imagename func>\n");
		exit(EXIT_FAILURE);
	}

	// create fat32 instance for processing
	fat32_instance = fs_create_instance(argv[1]);
	if (fat32_instance == NULL) {
		perror("Could not create the fat32 instance\n");
		exit(EXIT_FAILURE);
	}

	current_dir = fs_get_root(fat32_instance);
	char *buffer = argv[2];

	// Make the string uppercase
	int i;
	for (i = 0; i < strlen(buffer) + 1; i++) {
		buffer[i] = toupper(buffer[i]);
	}

	// print the info
	if (strncmp(buffer, CMD_INFO, strlen(CMD_INFO)) == 0) {
		print_info(fat32_instance);
	}

	// check for "LIST" cmd....
	// loop through the FAT and print the directories/files in tree like structure

	return 0;
} // close main


static uint32_t cluster_to_sector(FS_Instance *fat_fs, uint32_t cluster) {
  fat32BS *bs = (fat32BS *)fat_fs->image;

	return ((cluster - 2) * bs->BPB_SecPerClus) + fat_fs->FirstDataSector;
}


static int is_eoc(FS_Instance *fat_fs, uint32_t cluster) {
  return (FS_FAT32 == fat_fs->fs_type) ? (0x0FFFFFFF == cluster) : (FS_FAT16 == fat_fs->fs_type) ? (0xFFFF == cluster) : (0x0FFF == cluster);
}


static uint32_t iterate_chain(FS_Instance *fat_fs, uint32_t cluster) {
  assert(NULL != fat_fs);
  
  fat32BS *bs = (fat32BS *)fat_fs->image;

  // pp 15-17
  uint32_t fat_offset, this_fat_sec_num, this_fat_ent_offset;
  uint8_t *sec_buff;

  if (FS_FAT12 == fat_fs->fs_type)
    fat_offset = cluster + (cluster / 2);
  else if (FS_FAT16 == fat_fs->fs_type)
    fat_offset = cluster * 2;
  else
    fat_offset = cluster * 4;

  this_fat_sec_num = bs->BPB_RsvdSecCnt + (fat_offset / bs->BPB_BytesPerSec);
  this_fat_ent_offset = fat_offset % bs->BPB_BytesPerSec;
  sec_buff = fat_fs->image + this_fat_sec_num * bs->BPB_BytesPerSec;

  if (FS_FAT12 == fat_fs->fs_type) {
    if (cluster & 0x0001) {
      cluster = (*(uint16_t *)&sec_buff[this_fat_ent_offset]) >> 4;
    } else {
      cluster = (*(uint16_t *)&sec_buff[this_fat_ent_offset]) & 0x0fff;
    }
  } else {
     uint32_t fat_sz;
     if (bs->BPB_FATSz16 != 0)
       fat_sz = bs->BPB_FATSz16;
     else
       fat_sz = bs->BPB_FATSz32;
    
    if (FS_FAT16 == fat_fs->fs_type)
      cluster = *((uint16_t *) &sec_buff[this_fat_ent_offset]);
    else
      cluster = (*((uint32_t *) &sec_buff[this_fat_ent_offset])) & 0x0FFFFFFF;
  }
  
  return cluster;
}


static fatEntry *iterate_dir(FS_Instance *fat_fs, uint32_t *cluster, fatEntry *i, uint32_t *bytes_left) {
  assert(NULL != fat_fs);
  
  fat32BS *bs = (fat32BS *)fat_fs->image;
	int sector, first = 0;

	if (NULL == i) {
    *bytes_left = 0;
		first = 1;
	}
	
	do {
  	if (*bytes_left < sizeof(fatEntry)) {
      if (1 == *cluster) {
        // root on FAT12/16
        sector = bs->BPB_RsvdSecCnt + (bs->BPB_NumFATs * bs->BPB_FATSz16);
        if (first) {
          *bytes_left = fat_fs->RootDirSectors * bs->BPB_BytesPerSec;
        } else {
    			*bytes_left = 0;
    			return NULL;
        }
      } else {
        // chained directory
        sector = cluster_to_sector(fat_fs, *cluster);
        if (!first) {
          *cluster = iterate_chain(fat_fs, *cluster);
          if (is_eoc(fat_fs, *cluster)) {
      			*bytes_left = 0;
      			return NULL;
          }
          sector = cluster_to_sector(fat_fs, *cluster);
        }
        *bytes_left = bs->BPB_SecPerClus * bs->BPB_BytesPerSec;
      }
  		i = (void *)bs + sector * bs->BPB_BytesPerSec;
  		first = 1;
		}
		if (!first) {
			i++;
		}
		*bytes_left = *bytes_left - sizeof(fatEntry);
	} while ((i->DIR_Name[0] > 0x00 && i->DIR_Name[0] <= 0x20) || 0xe5 == i->DIR_Name[0]);
	
	if (0x00 == i->DIR_Name[0]) {
		return NULL;
	}
	
	return i;
}


static uint32_t free_sectors(FS_Instance *fat_fs) {
  uint32_t free_count = 0;

  fat32BS *bs = (fat32BS *)fat_fs->image;
  
  for (uint32_t cluster = 0; cluster < fat_fs->DataSec / bs->BPB_SecPerClus; cluster++) {
    if (0 == iterate_chain(fat_fs, cluster + FIRST_DATA_CLUSTER)) {
      free_count++;
    }
  }
  return free_count;
} // free_sectors


FS_Instance *fs_create_instance(char *image_path) {
	assert(NULL != image_path);

	FS_Instance *fat_fs;
	int fd;
	struct stat fd_stat;
	uint8_t *image;

	fd = open(image_path, O_RDONLY);
	if (fd < 0) {
	perror("Error opening file");
	return NULL;
	}

	if (fstat(fd, &fd_stat) < 0) {
	perror("Error getting file information");
	return NULL;
	}

	image = mmap(0, fd_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (NULL == image) {
	perror("Error mapping file");
	return NULL;
	}

	fat_fs = malloc(sizeof(FS_Instance));
	fat_fs->image_path = malloc(strlen(image_path + 1));
	strcpy(fat_fs->image_path, image_path);
	fat_fs->image = image;

	fat32BS *bs = (fat32BS *)image;
	uint32_t FATSz, TotSec, CountofClusters;

	FATSz = bs->BPB_FATSz32;
	TotSec = bs->BPB_TotSec32;

	fat_fs->RootDirSectors = ((bs->BPB_RootEntCnt * 32) + (bs->BPB_BytesPerSec - 1)) / bs->BPB_BytesPerSec;
	fat_fs->FirstDataSector = bs->BPB_RsvdSecCnt + (bs->BPB_NumFATs * FATSz) + fat_fs->RootDirSectors;
	fat_fs->DataSec = TotSec - (bs->BPB_RsvdSecCnt + (bs->BPB_NumFATs * FATSz) + fat_fs->RootDirSectors);
	CountofClusters = fat_fs->DataSec / bs->BPB_SecPerClus;

	if (CountofClusters < 4085) {
		/* Volume is FAT12 */
		fat_fs->fs_type = FS_FAT12;
	} else if(CountofClusters < 65525) {
	/* Volume is FAT16 */
		fat_fs->fs_type = FS_FAT16;
	} else {
	/* Volume is FAT32 */
		fat_fs->fs_type = FS_FAT32;
	}

	fatEntry *dir = NULL;
	uint32_t bytes_left;
	FS_CurrentDir root = fs_get_root(fat_fs);

	fat_fs->volume_id[0] = '\0';
	while (NULL != (dir = iterate_dir(fat_fs, &root, dir, &bytes_left))) {
		if (ATTR_VOLUME_ID == dir->DIR_Attr) {
			strncpy(fat_fs->volume_id, (char *)dir->DIR_Name, DIR_Name_LENGTH);
			fat_fs->volume_id[DIR_Name_LENGTH] = '\0';
			break;
		}
	}

	return fat_fs;
}


FS_CurrentDir fs_get_root(FS_Instance *fat_fs) {
	// TODO: FAT32
  assert(NULL != fat_fs);
  
  if (FS_FAT32 == fat_fs->fs_type) {
    fat32BS *bs = (fat32BS *)fat_fs->image;
    return bs->BPB_RootClus;
  } 
  return -1;
}


void print_info(FS_Instance *fat_fs) {
	assert(NULL != fat_fs);

	fat32BS *bs = (fat32BS *)fat_fs->image;

	printf("\nVolume information for: %s\n", fat_fs->image_path);

	printf("\nDisk information:\n");
	printf("-----------------\n");

	printf("OEM Name: %.8s\n", bs->BS_OEMName);
	printf("Volume Label: %.11s\n", bs->BS_VolLab);
	printf("File System Type (text): %.8s\n", bs->BS_FilSysType);
	printf("Media Type: 0x%X (%s)\n", bs->BPB_Media, (MEDIA_FIXED == bs->BPB_Media)?"fixed":(MEDIA_REMOVABLE == bs->BPB_Media)?"removable":"unknown");

	long long totalB = (0 == bs->BPB_TotSec32) ? bs->BPB_TotSec16 : bs->BPB_TotSec32;
	totalB *= bs->BPB_BytesPerSec;
	double totalMB = totalB/1000000.0;
	printf("Size: %lld bytes (%.2fMB)\n", totalB, totalMB);

	printf("\nDisk geometry:\n");
	printf("--------------\n");

	printf("Bytes Per Sector: %d\n", bs->BPB_BytesPerSec);
	printf("Sectors Per Cluster: %d\n", bs->BPB_SecPerClus);
	printf("Total Sectors: %d\n", (0 == bs->BPB_TotSec32) ? bs->BPB_TotSec16 : bs->BPB_TotSec32);
	printf("Physical - Sectors per Track: %d\n", bs->BPB_SecPerTrk);
	printf("Physical - Heads: %d\n", bs->BPB_NumHeads);

	printf("\nFile system info:\n");
	printf("----------------\n");

	printf("Volume ID: %s\n", fat_fs->volume_id);
	printf("File System Type (calculated): %s\n", (FS_FAT12 == fat_fs->fs_type)?"FAT12":(FS_FAT16 == fat_fs->fs_type)?"FAT16":"FAT_32");
	printf("FAT Size (sectors): %d\n", bs->BPB_FATSz32);
	//printf("Free space: %d bytes\n", free_sectors(fat_fs) * bs->BPB_BytsPerSec);
}
