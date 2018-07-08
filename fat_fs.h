#ifndef FAT_FS_H
#define FAT_FS_H

#include <inttypes.h>

#define DIR_Name_LENGTH 11
#define CMD_INFO "INFO"
#define FName_LENGTH (DIR_Name_LENGTH + 1)
#define RootName_LENGTH 8
#define MAX_TIMESTAMP_STR 20
#define EPOCH 1980
#define ATTR_LONG_NAME_MASK (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE)

#define FIRST_DATA_CLUSTER 2

#define MEDIA_FIXED 0xF8
#define MEDIA_REMOVABLE 0xF0
#define DRV_NUM_FLOP 0x00
#define DRV_NUM_HD 0x80

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)


typedef enum {
	FS_FAT12,
	FS_FAT16,
	FS_FAT32
} FS_Type;

struct FS_Instance_struct {
  char *image_path;
  uint8_t *image;

	FS_Type fs_type;
	
	uint32_t RootDirSectors;
	uint32_t FirstDataSector;
	uint32_t DataSec;
	
	char volume_id[DIR_Name_LENGTH + 1];
};

struct fatEntry_struct {
		uint8_t DIR_Name[DIR_Name_LENGTH];
		uint8_t DIR_Attr;
		uint8_t DIR_NTRes;
		uint8_t DIR_CrtTimeTenth;
		uint16_t DIR_FstClusHI;
		uint16_t DIR_FstClusLO;
		uint32_t DIR_FileSize;
};

typedef struct FS_Instance_struct FS_Instance;
typedef uint32_t FS_CurrentDir;
typedef struct fatEntry_struct fatEntry;

FS_Instance *fs_create_instance(char *image_path);
FS_CurrentDir fs_get_root(FS_Instance *fat_fs);

void print_info(FS_Instance *fat_fs);

void fs_cleanup(FS_Instance *fat_fs);

#endif
