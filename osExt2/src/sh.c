#include <string.h>
#include <stdio.h>
#include "ext2.h"
#include "disk.h"


int getcmd(char *buf, int nbuf);
void setargs(char *cmd, char* argv[],int *argc);
int runcmd(char*argv[],int argc);

#define MAXLINE 256
#define MAXARG 32

//各种各样的空格
char whitespace[] = " \t\r\n\v";

 
 // 获取用户输入的命令
int getcmd(char *buf, int nbuf)
{
    memset(buf, 0, nbuf);
    fgets(buf, nbuf, stdin);
    if(buf[0] == 0)   return -1;
    buf[strlen(buf) - 1] = 0; 
    return 0;
}

 // 设置参数
void setargs(char* cmd, char** argv, int* argc)
{
    int i = 0, j = 0;  
    // 每次循环找到参数
    // 让argv[i]分别指向参数的开头，并且将参数后面的空格设为\0，这样读取的时候可以直接读
    for (; cmd[j] != '\n' && cmd[j] != '\0'; j++)
    {
        while (strchr(whitespace,cmd[j])){ // 判断cmd[j]中有没有whitespace中的内容（即各种各样的空格），跳过
            j++;
        }
        argv[i++]=cmd+j;  //让argv[i]指向参数
        while (strchr(whitespace,cmd[j])==0){// 找到参数后面的单空格，0表示空指针，strchr的返回值类型是char*
            j++;
        }
        // 把找到的参数后面的第一个空格设置为结束符
        cmd[j]='\0';
    }
    argv[i]=0;  //结束
    *argc=i;    //argc表示有几个参数
}
 


// 执行命令
int runcmd(char*argv[],int argc)
{
    //ls
    char *cmd = "ls";
    if (!strcmp(argv[0],cmd))
    {
      uint32_t inode_id;
      if (argc==1)
        inode_id = getInodeIdByPath("");
      else{
        char* path = argv[1];
        inode_id = getInodeIdByPath(path);
      }
      PrintItems(inode_id);
      return 1;
    }

    //mkdir
    cmd = "mkdir";
    if (!strcmp(argv[0],cmd))
    {
      if (!sp.free_inode_count){
        printf("No space left！\n");
        return 1;
      }
      char *path = argv[1];
      char* start = getLast(path);  //找到最后一个文件夹的起始位置
      *(start-1) = 0;               //路径截掉最后一个目录
      uint32_t inode_id = getInodeIdByPath(path); //找到前面路径的inode号
      //确保前面的目录存在且最后一个目录（要创建的）不存在
      if (inode_id==-1 || getDirItem(inode_id,start) != -1){
        printf("invalid path!\n");
        return 1;
      }
      createNew(1,inode_id,start);  //创建文件夹，1表示文件夹
      return 1;
    }
    

    //touch
    cmd = "touch";
    if (!strcmp(argv[0],cmd))
    {
      if (!sp.free_inode_count){
        printf("No space left！\n");
        return 1;
      }
      char* path = argv[1];
      char* start = getLast(path); //找到最后一个文件的起始位置      
      *(start-1) = 0;             //路径截掉最后一个文件，找到前面路径的inode号
      uint32_t inode_id = getInodeIdByPath(path);

      //确保前面的目录存在且最后一个文件（要创建的）不存在
      if (inode_id==-1 || getDirItem(inode_id,start) != -1){
        printf("invalid path!\n");
        return 1;
      }
      createNew(0,inode_id,start);  //创建文件，0表示文件
      return 1;
    }

    //cp
    cmd = "cp";
    //复制文件不复制inode，复制目录项，将inode.link+1(硬链接)。
    if (!strcmp(argv[0],cmd))
    {
      char* path1 = argv[1];
      char* path2 = argv[2];
      uint32_t inode_id_1 = getInodeIdByPath(path1);
      if (inode_id_1==-1){
        printf("The file doesn't exit!\n");
        return 1;
      }
      char* start = getLast(path2); //找到最后一个文件的起始位置
      *(start-1) = 0;   //路径截掉最后一个文件，找到前面路径的inode号
      uint32_t inode_id_2 = getInodeIdByPath(path2);
      if (inode_id_2 == -1){
        printf("invalid path!\n");
        return 1;
      }
      if (getDirItem(inode_id_2,start)!=-1){
        printf("%s already exits!\n",path2);
        return 1;
      }
      copyFile(inode_id_1, inode_id_2,start);
      return 1;
    }

    //shutdown
    cmd = "shutdown";
    if (!strcmp(argv[0],cmd))
    {
      shutDownEXT2();
      printf("Successfully Close the file system!\n");
      return 0;
    }
  
}
 

int main()
{
    // buf存储用户输入的命令
    char buf[MAXLINE] = "n";
    while (memcmp(buf,"y",2) !=0 )
    {
      printf("Would you like to open the file system?(y/n):");
      getcmd(buf,MAXLINE);
    }
    if (initExt2()==-1)
    {
        printf("failed to initialize the file system!\n");
        return -1;
    }
    printf("The file system has been initialized, have fun!\n");

    printf("o(*￣▽￣*)ブ：");
    // 执行getcmd获得用户输入的命令
    while (getcmd(buf,MAXLINE) >= 0)
    {
      char* argv[MAXARG];
      int argc=0;
      // 重新设置参数格式
      setargs(buf, argv, &argc);
      //运行命令
      if(!runcmd(argv,argc)) break;
      memset(buf, 0, MAXLINE);
      printf("o(*￣▽￣*)ブ：");
    }
    return 0;
}