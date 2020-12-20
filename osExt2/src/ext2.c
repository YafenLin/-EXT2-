#include "disk.h"
#include "ext2.h"
#include <string.h>
#include <stdio.h>
#define ML 10   //最多进10层文件夹

/**
 * 找到路径中最后一个文件夹/文件在path中的起始位置
 * @return 指向最后一个文件夹/文件起始位置的指针
*/
char* getLast(char* path){
    char* start = path;
    int j = 0;     
    while (path[j] != 0){
        while (path[j] != '/' && path[j] != 0){ // 找到文件夹后面的斜杠
            j++;
        }
        if (path[j] == 0){  // 文件夹后面没有斜杠（最后一个文件夹）
            return start;
        }
        j++;
        start = path+j;
    }
}


/**
 * 分配一个空闲的数据块，注意dataBlock号从33开始
 * @return 空闲数据块的块号
 */
uint32_t allocNewDataBlock(){
    if (!sp.free_block_count){  //如果没有空闲数据块
        printf("No space left！\n");
        return -1;            
    }
    sp.free_block_count--;
    for (int i = 0; i < 128; i++){
        int j;
        uint32_t map = sp.block_map[i];
        for (j = 0; j < 32 && map &0x01; j++)
            map = map >> 1;
        if (j < 32){
            sp.block_map[i] = sp.block_map[i] | (1 << j);
            return 32*i + j + 33;
        }
    }
}

/**
 * 分配一个空闲inode
 * @return 空闲inode号
 */
uint32_t allocNewInode(){
    //这里不用做freeinode的检查，因为在sh.c中已经检查。如果能调用，说明就有空闲的inode
    sp.free_inode_count--;
    for (int i = 0; i < 32; i++){
        int j;
        uint32_t map = sp.inode_map[i];
        for (j = 0; j < 32 && map &0x01; j++)
            map = map >> 1;
        if (j < 32){
            sp.inode_map[i] = sp.inode_map[i] | (1 << j);
            return 32*i + j;
        }
    }
}

/**
 * 在indoe_id对应的目录里找name
 * @return 找到的name的inode_id，没找到返回-1
*/
uint32_t getDirItem(uint32_t inode_id, char* name){
    uint32_t pointer;
    Dir_item dir_item;
    for (int i = 0; i < 6 && inode[inode_id].block_point[i]!=0; i++){
        pointer = inode[inode_id].block_point[i]; //pointer存储目录的数据块块号
        read_data_block(pointer,buffer);    //从磁盘读取数据块
       
        //遍历读出来的数据块的每个目录项 
        for(int j = 0; j < 8; j++){
            //找到name的目录项
            memcpy(&dir_item,buffer + j*sizeof(Dir_item),sizeof(Dir_item));
            if(dir_item.valid){
                if (!memcmp(name,dir_item.name,strlen(name)+1))
                {
                    return dir_item.inode_id;
                }
            }
        }
    }
    return -1;
}

void PrintItems(uint32_t inode_id){
    uint32_t pointer;
    // Dir_item* dir_item;
    Dir_item dir_item;
    for (int i = 0; i < 6 && inode[inode_id].block_point[i]!=0; i++){
        pointer = inode[inode_id].block_point[i]; //pointer存储根目录的数据块块号
        read_data_block(pointer,buffer);    //从磁盘读取数据块
        //遍历读出来的数据块的每个目录项
        //dir_item = (Dir_item*)buffer;
        for(int j = 0; j < 8; j++){
            //找到name的目录项
            memcpy(&dir_item,buffer+j*sizeof(Dir_item),sizeof(Dir_item));
            if(dir_item.valid){
                //printf("%s",dir_item->name);
                printf("%s\n",dir_item.name);
            }
            //dir_item += sizeof(Dir_item);
        }
    }
}


/**
 * 根据path递归寻找得到对应的inode号
 * @return 路径对应的inode号
 */
uint32_t getInodeIdByPath(char*path){

    if (!strlen(path))    //根目录
        return 0;   
    char* start = getLast(path);    
    *(start-1) = 0;
    uint32_t inode_id = getInodeIdByPath(path); 
    uint32_t new_inode_id = getDirItem(inode_id,start); 
    return new_inode_id;
}



/**
 * 创建文件/文件夹
 * @param type 要创建的类型，0：文件，1：文件夹 
 * @param inode_id 目录的inode号
 * @param name 要创建的文件/文件夹的名称  
 * k
 */
void createNew(uint8_t type, uint32_t inode_id, char* name){
    uint32_t pointer;
    Dir_item dir_item;
    //先创建一个新的inode。

    uint32_t new_inode_id = allocNewInode();
    inode[new_inode_id].link=1;
    inode[new_inode_id].size=0;
    inode[new_inode_id].file_type=type;
    inode[new_inode_id].block_point[0]=0;   //空的文件和文件夹

    //再填写目录项
    //i实现对inode_id数据块的遍历
    for (int i = 0; i < 6; i++){
        pointer = inode[inode_id].block_point[i]; //pointer存储根目录的数据块块号
        if (!pointer)   //如果有效数据块全满or没有有效数据块了,分配一个新的数据块
        {
            uint32_t dataBlock = allocNewDataBlock();
            inode[inode_id].block_point[i] = dataBlock;
            memset(buffer,0,1024);
            //dir_item = (Dir_item*)buffer;
            memcpy(&dir_item,buffer,sizeof(Dir_item));
            dir_item.valid=1;
            dir_item.type=type;
            memcpy(dir_item.name,name,strlen(name)+1);
            dir_item.inode_id = new_inode_id;
            memcpy(buffer,&dir_item,sizeof(Dir_item));
            write_data_block(dataBlock,buffer);
            return;
        }

        read_data_block(pointer,buffer);    //从磁盘读取数据块
        //遍历读出来的数据块的每个目录项
        
        for(int j = 0; j < 8; j++){
            //找到无效的目录项
            memcpy(&dir_item,buffer+j*sizeof(Dir_item),sizeof(Dir_item));
            if(!dir_item.valid){
                Dir_item new_dir_item;
                new_dir_item.inode_id = new_inode_id;
                memcpy(new_dir_item.name,name,sizeof(name));
                new_dir_item.type = type;
                new_dir_item.valid = 1;
                memcpy(buffer+j*sizeof(Dir_item),&new_dir_item,sizeof(new_dir_item));
                write_data_block(pointer,buffer);
                return ;
            }
        }
    }
    
    
}



// /**
//  * 获取路径中包含的文件夹
//  * 返回文件夹的数量
//  */
// int getDirsInPath(char* path){
//     // 表示第i个文件夹
//     int i = 0; 
//     // 用j来遍历path
//     int j = 0;  
//     int num = 0;    
//     // 每次循环找到文件夹
//     // 默认从root开始，root前面没有斜杠,格式为root/dir1/dir2
//     // dirsInPath[i]分别指向文件夹的开头，并且将文件夹后面的斜杠设为\0，这样读取的时候可以直接读
//     while (path[j] != 0)
//     {
//         //dirsInPath[i]指向文件夹
//         num++;
//         dirsInPath[i++]=path+j;

//         // 找到文件夹后面的斜杠
//         while (path[j] != '/' && path[j] != 0){
//             j++;
//         }
//         // 文件夹后面没有斜杠（最后一个文件夹）
//         if (path[j] == 0)
//         {
//             dirsInPath[i]=0;  //结束
//             return num;
//         }
//         // 把找到的文件夹后面的斜杠设置为结束符
//         path[j]='\0';
//         j++;
//     }
// }

// /**
//  * 读取传进来的目录的内容到content数组中
//  * @param path 要读取内容的目录路径
//  * @return 目录中文件/文件夹的数量
//  */
// uint32_t getDirContent(uint32_t inode_id){
//     //i实现对inode_id的有效数据块的遍历
//     uint32_t pointer;
//     Dir_item* dir_item;
//     uint32_t contentNum=0;
//     for (int i = 0; i < 6 && inode[inode_id].block_point[i]!=0 ; i++)
//     {
//         pointer = inode[inode_id].block_point[i];
//         read_data_block(pointer,buffer);    //从磁盘读取数据块
//         dir_item = (Dir_item*)buffer;
//         //一个数据块有8个目录项
//         for(int j = 0; j < 8; j++){
//             if (dir_item->valid)
//             {
//                 memcpy(content[contentNum++],dir_item->name,sizeof(dir_item->name));
//             }
//             dir_item += sizeof(Dir_item);
//         }
//     }
//     return contentNum;
// }

/**
 * 复制inode_id_1对应的文件到inode_id_2对应的目录中，并取名为name
*/
void copyFile(uint32_t inode_id_1, uint32_t inode_id_2, char* name){
    uint32_t pointer;
    Dir_item dir_item;

    //填写inode_id_2的目录项
    //i实现对inode_id_2数据块的遍历
    for (int i = 0; i < 6; i++){
        pointer = inode[inode_id_2].block_point[i]; //pointer存储目录的数据块块号
        if (!pointer)   //如果有效数据块全满or没有有效数据块了,分配一个新的数据块
        {
            uint32_t dataBlock = allocNewDataBlock();
            inode[inode_id_2].block_point[i] = dataBlock;
            memset(buffer,0,1024);
            //dir_item = (Dir_item*)buffer;
            memcpy(&dir_item,buffer,sizeof(Dir_item));
            dir_item.valid=1;
            dir_item.type=0;
            memcpy(dir_item.name,name,sizeof(name));
            dir_item.inode_id = inode_id_1;
            inode[inode_id_1].link++;   //连接数+1
            memcpy(buffer,&dir_item,sizeof(Dir_item));
            write_data_block(dataBlock,buffer);
            return;
        }

        read_data_block(pointer,buffer);    //从磁盘读取数据块
        //遍历读出来的数据块的每个目录项
        for(int j = 0; j < 8; j++){
            //找到无效的目录项
            memcpy(&dir_item,buffer+j*sizeof(Dir_item),sizeof(Dir_item));
            if(!dir_item.valid){
                Dir_item new_dir_item;
                new_dir_item.inode_id = inode_id_1;
                inode[inode_id_1].link++;
                memcpy(new_dir_item.name,name,sizeof(name));
                new_dir_item.type = 0;
                new_dir_item.valid = 1;
                memcpy(buffer+j*sizeof(Dir_item),&new_dir_item,sizeof(new_dir_item));
                write_data_block(pointer,buffer);
                return ;
            }
        }
    }
}


/**
 * 读取一个数据块(1数据块v2磁盘块)
 * @param block_num 数据块号数
 * @param buf       读取的数据块的存储区
 * 
 * */
void read_data_block(unsigned int block_num, char* buf)
{
    disk_read_block(2*block_num,buf);
    disk_read_block(2*block_num+1,buf+DEVICE_BLOCK_SIZE);
}

/**
 * 写一个数据块(1数据块v2磁盘块)
 * @param block_num 数据块号数
 * @param buf       写入的数据块
 * 
 * */
void write_data_block(unsigned int block_num, char* buf)
{
    disk_write_block(2*block_num,buf);
    disk_write_block(2*block_num+1,buf+DEVICE_BLOCK_SIZE);
}




/** 
 * 打开EXT2文件系统
 * 先从磁盘把超级块和inode数组读进来（退出文件系统的时候再写回）
 * 如果是第一次打开文件系统要初始化超级块
 * 成功返回0，失败返回-1
 * */ 
int initExt2()
{
    //打开磁盘失败
    if (open_disk() != 0)
    {
        return -1;
    }
    // 读取超级块
    read_data_block(0, buffer);
    memcpy(&sp, buffer, sizeof(sp_block));  //超级块的长度不等于一个block
    
    //如果是第一次打开文件系统要初始化超级块
    if(sp.magic_num != 0xdec0d0){ 
        printf("Open this file system for the first time\n");
        sp.magic_num = 0xdec0d0;
        sp.free_block_count = 4063; // 根目录消耗一个 
        sp.free_inode_count = 1023; // 一个指向根目录
        sp.dir_inode_count = 1;     //目录inode数
        memset(sp.block_map, 0, 128);
        memset(sp.inode_map, 0, 32);

        // 初始化inode数组
        memset(inode, 0, sizeof(inode));
        // 初始化根目录（0号inode）
        inode[0].file_type = 1; //文件夹
        inode[0].link = 1;
        sp.inode_map[0] = 1;   //0号inode被根目录占用 

        return 0;
    }

    // 读取inode数组（1~32号数据块）
    for(int i=0; i<32; i++){ 
        read_data_block(i+1, buffer);
        //一个数据块1KB = 32个inode
        memcpy(&inode[32*i],buffer,sizeof(Inode)*32);
    }
    return 0;
}

/**
 * 关闭文件系统
*/

void shutDownEXT2(){
    memset(buffer, 0, 1024);
    memcpy(buffer, &sp, sizeof(sp_block));
    write_data_block(0,buffer);
    for (int i = 0; i < 32; i++)
    {
        memcpy(buffer, &inode[i*32], sizeof(buffer));
        write_data_block(i+1,buffer);
    }
    close_disk();    
}

