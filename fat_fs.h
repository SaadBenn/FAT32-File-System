#pragma pack(push)
#pragma pack(1)

// For reading files in our file system
typedef struct Dir_Info {
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} Dir_Info;

#pragma pack(pop)


// Structure for storing file info
#pragma pack(push)
#pragma pack(1)

typedef struct FS_Info {
    uint32_t FSI_LeadSig;
    char FSI_Rerserved1[480];
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count;
    uint32_t FSI_Nxt_Free;
    char FSI_Reserved2[12];
    uint32_t FSI_TrailSig;
} FS_Info;

#pragma pack(pop)


// printing functions
int print_dir_recursively(fat32BS *, uint32_t, uint32_t);
void print_info(fat32BS *);
void print_contents_of_the_file(fat32BS *, uint32_t, char *);
void print_dash(int);

// validate the directory
bool is_directory(Dir_Info *currDir);

// cluster management functions
uint32_t check_chain(fat32BS *, uint32_t);
void traverse_for_file(fat32BS *bpb, uint32_t);
uint32_t get_sector_of_cluster(fat32BS *, uint32_t);

char *tokenize_dir_file_name(char *);
char *seek_and_read_file_contents(uint32_t, uint32_t);
char *concat_extension_to_file(char *, char *);
char *name_copier(char *, bool);
