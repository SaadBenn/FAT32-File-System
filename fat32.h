
//FAT (File Allocation Table)

#ifndef FAT32_H
#define FAT32_H

#include <inttypes.h>

/* boot sector constants */
#define BS_OEMName_LENGTH 8
#define BS_VolLab_LENGTH 11
#define BS_FilSysType_LENGTH 8

#pragma pack(push)
#pragma pack(1)

//BPB = BIOS Parameter Block; First sector of a volume.
// For Defining the chracteristics of our BIOS cluster
struct fat32BS_struct {
    char BS_jmpBoot[3];
    char BS_OEMName[BS_OEMName_LENGTH]; //Volume Name
    uint16_t BPB_BytesPerSec; //Count of bytes per sector
    uint8_t BPB_SecPerClus; //Number of sector per allocation units
    uint16_t BPB_RsvdSecCnt; //# of reserved sector
    uint8_t BPB_NumFATs; // # if FAT data structures
    uint16_t BPB_RootEntCnt; //Directory entries in the root directory.
    uint16_t BPB_TotSec16; //16 bit total count of sectors
    uint8_t BPB_Media; // value for fixed media(0xF8)
    uint16_t BPB_FATSz16; //this is 0 in 32 bit FAT
    uint16_t BPB_SecPerTrk; //Sectors per track for interrupt
    uint16_t BPB_NumHeads; //# of heads for interrupt
    uint32_t BPB_HiddSec; //# of hidden sector
    uint32_t BPB_TotSec32; //# of sectors in the volume (32 bit)
    uint32_t BPB_FATSz32; //32-bit count of sectors occupied by ONE FAT
    uint16_t BPB_ExtFlags;
    uint8_t BPB_FSVerLow;
    uint8_t BPB_FSVerHigh;
    uint32_t BPB_RootClus; // Cluster number of the first cluster
    uint16_t BPB_FSInfo; //Sector # if FSINFO structure in the reserved area
    uint16_t BPB_BkBootSec;//if != 0, copy of root directory
    char BPB_reserved[12]; //Reserved for future expansion
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    char BS_VolLab[BS_VolLab_LENGTH];
    char BS_FilSysType[BS_FilSysType_LENGTH]; // "FAT32"
    char BS_CodeReserved[420];
    uint8_t BS_SigA;
    uint8_t BS_SigB;
};
#pragma pack(pop)

typedef struct fat32BS_struct fat32BS;

#endif
