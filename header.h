#ifndef OS_FILESYSTEM_HEADER_H
#define OS_FILESYSTEM_HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <time.h>

/**
 * 宏定义
 */
#define  TRUE               1           //TRUE 定义
#define  CMD_LENGTH         30          //命令行缓冲区大小
#define  BLOCK_SIZE         1024        //磁盘块大小
#define  SIZE               1024000     //虚拟磁盘空间大小
#define  END                65535       //FAT中的文件结束标志
#define  FREE               0           //FAT中盘块空闲标志
#define  ROOT_BLOCK_NUM     2           //根目录区所占盘块总数
#define  MAX_OPEN_FILE      10          //最多同时打开文件个数

/**
 * 私有时间类型定义
 */
typedef struct TIME{
    int hour;
    int minute;
    int second;
} my_time;

/**
 * 私有日期类型定义
 */
typedef struct DATE{
    int year;
    int month;
    int day;
} my_date;

/**
 * 仿照 FAT16 设置的文件控制块结构
 */
typedef struct FCB {
    char filename[8];                   //文件名
    char extname[3];                    //文件仿照 FAT16 设置的
    unsigned char attribute;            //文件属性字段,为简单起见，我们只为文件设置了两种属性：值为 0 时表示目录文件，值为 1 时表示数据文件
    my_time time;                       //文件创建时间
    my_date date;                       //文件创建日期
    unsigned short first;               //文件起始盘块号
    unsigned long length;               //文件长度（字节数）
    char free;                          //表示目录项是否为空，若值为 0，表示空，值为 1，表示已分配
} fcb;

/**
 * 文件分配表结构
 */
typedef struct FAT {
    unsigned short id;
} fat;

/**
 * 用户打开文件表结构
 */
typedef struct USER_OPEN {
    char filename[8];                   //文件名
    char extname[3];                    //文件扩展名
    unsigned char attribute;            //文件属性：值为 0 时表示目录文件，值为 1 时表示数据文件
    my_time time;                       //文件创建时间
    my_date date;                       //文件创建日期
    unsigned short first;               //文件起始盘块号
    unsigned long length;               //文件长度（对数据文件是字节数，对目录文件可以是目录项个数）
    char free;                          //表示目录项是否为空，若值为 0，表示空，值为 1，表示已分配
    int dir_no;                         //相应打开文件的目录项在父目录文件中的盘块号
    int dir_off;                        //相应打开文件的目录项在父目录文件的 dir_no 盘块中的目录项序号
    char dir[MAX_OPEN_FILE][80];        //相应打开文件所在的目录名，这样方便快速检查出指定文件是否已经打开
    int count;                          //读写指针在文件中的位置
    char fcb_state;                     //是否修改了文件的 FCB 的内容，如果修改了置为 1，否则为 0
    char t_openfile;                    //表示该用户打开表项是否为空，若值为0，表示为空，否则表示已被某打开文件占据
} user_open;

/**
 * 引导块结构
 */
typedef struct BLOCK0 {
    char magic_number[8];               //文件系统的魔数
    char information[200];              //存储一些描述信息，如磁盘块大小、磁盘块数量、最多打开文件数等
    unsigned short root;                //根目录文件的起始盘块号
    unsigned char *start_block;         //虚拟磁盘上数据区开始位置
} block0;

/**
 * 全局变量定义
 */
FILE *fptr;                             //文件指针
char general_buffer[SIZE];              //通用缓冲区
unsigned char *my_v_hard;               //指向虚拟磁盘的起始地址
user_open open_file_list[MAX_OPEN_FILE];//用户打开文件表数组
user_open *ptr_current_dir;             //指向用户打开文件表中的当前目录所在打开文件表项的位置
char current_dir[80];                   //记录当前目录的目录名（包括目录的路径）
unsigned char *start_position;          //记录虚拟磁盘上数据区开始位置

/**
 * 函数声明
 */
void start_sys();

void my_format();

void my_cd(char *dirname);

void my_mkdir(char *dirname);

void my_rmdir(char *dirname);

void my_ls(void);

int my_create(char *filename);

void my_rm(char *filename);

int my_open(char *filename);

void my_close(int fd);

int my_write(int fd);

int do_write(int fd, char *text, int len, char w_style);

int my_read(int fd, int len);

int do_read(int fd, int len, char *text);

void my_exit_sys();

void init_file_system();

void init_cmd();

#endif
