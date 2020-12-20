#ifndef EXT2_H
#define EXT2_H

typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
typedef signed char __int8_t;
typedef unsigned char __uint8_t;

typedef __uint8_t int8_t;
typedef __uint16_t int16_t;
typedef __uint32_t int32_t;
typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;

//超级块
typedef struct super_block {
    int32_t magic_num;                  // 幻数
    int32_t free_block_count;           // 空闲数据块数
    int32_t free_inode_count;           // 空闲inode数
    int32_t dir_inode_count;            // 目录inode数
    uint32_t block_map[128];            // 数据块占用位图
    uint32_t inode_map[32];             // inode占用位图
} sp_block;

//索引节点（一个inode对应一个文件/目录）
typedef struct inode {
    uint32_t size;              // 文件大小
    uint16_t file_type;         // 文件类型（文件/文件夹）
    uint16_t link;              // 连接数
    uint32_t block_point[6];    // 数据块指针
} Inode;

//目录项
typedef struct dir_item {               // 目录项一个更常见的叫法是 dirent(directory entry)
    uint32_t inode_id;          // 当前目录项表示的文件/目录的对应inode
    uint16_t valid;             // 当前目录项是否有效 
    uint8_t type;               // 当前目录项类型（文件/目录）
    char name[121];             // 目录项表示的文件/目录的文件名/目录名
} Dir_item;

sp_block sp;    //超级块
Inode inode[1024]; // inode数组

char buffer[1024];  //从磁盘读取的块的缓冲区1KB

char* getLast(char* path);
uint32_t getInodeIdByPath(char*path);
uint32_t getDirItem(uint32_t inode_id, char* name);
void PrintItems(uint32_t inode_id);

void read_data_block(unsigned int block_num, char* buf);
void write_data_block(unsigned int block_num, char* buf);

int initExt2();
void shutDownEXT2();

void createNew(uint8_t type, uint32_t inode_id, char* name);
void copyFile(uint32_t inode_id_1, uint32_t inode_id_2, char* name);

uint32_t allocNewDataBlock();
uint32_t allocNewInode();

#endif 
