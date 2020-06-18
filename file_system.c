#include "header.h"

/**
 * myfsys 文件系统实体文件路径名
 */
char *SYS_FILE = "myfsys";

/**
 * 文件系统初始化
 */
void start_sys() {
    unsigned char buffer[SIZE];
    unsigned char flag[8] = {'1', '0', '1', '0', '1', '0', '1', '0'};
    my_v_hard = (unsigned char *) malloc(SIZE);

    if (fopen_s(&fptr, SYS_FILE, "r") == 0) {
        fread(buffer, SIZE, 1, fptr);
        if (memcmp(flag, buffer, sizeof(flag)) == 0) {
            memcpy(my_v_hard, buffer, SIZE);
        } else {
            init_file_system();
        }
    } else {
        init_file_system();
    }

    fwrite(my_v_hard, SIZE, 1, fptr);
    fclose(fptr);

    fcb *root = (fcb *) (my_v_hard + BLOCK_SIZE * 5);
    user_open *current = &open_file_list[0];
    strcpy_s(current->filename, sizeof(root->filename), root->filename);
    strcpy_s(current->extname, sizeof(root->extname), root->extname);
    current->attribute = root->attribute;
    current->time = root->time;
    current->date = root->date;
    current->first = root->first;
    current->length = root->length;
    current->free = root->free;
    current->dir_no = 5;
    current->dir_off = 0;
    strcpy_s(current->dir[0], sizeof("/"), "/");
    current->count = 0;
    current->fcb_state = 0;
    current->t_openfile = 1;

    ptr_current_dir = current;
    start_position = ((block0 *) my_v_hard)->start_block;
    strcpy_s(current_dir, sizeof("/"), "/");
}

/**
 * 磁盘格式化
 */
void my_format() {
    int i;
    block0 *boot = (block0 *) my_v_hard;
    time_t now;
    struct tm now_time;
    time(&now);
    localtime_s(&now_time, &now);
    char info[] = "189050712 过昊天 简单 FAT 文件系统";

    strcpy_s(boot->magic_number, sizeof("10101010"), "10101010");
    strcpy_s(boot->information, sizeof(info), info);
    boot->root = 5;
    boot->start_block = my_v_hard + BLOCK_SIZE * 5;

    fat *fat1 = (fat *) (my_v_hard + BLOCK_SIZE);
    fat *fat2 = (fat *) (my_v_hard + BLOCK_SIZE * 3);

    for (i = 0; i < 5; i++) {
        fat1[i].id = END;
        fat2[i].id = END;
    }
    fat1[5].id = 6;
    fat1[6].id = END;
    fat2[5].id = 6;
    fat2[6].id = END;
    for (i += ROOT_BLOCK_NUM; i < (SIZE / BLOCK_SIZE); i++) {
        fat1[i].id = FREE;
        fat2[i].id = FREE;
    }

    fcb *ptr_current_dir_fcb = (fcb *) (my_v_hard + BLOCK_SIZE * 5);
    strcpy_s(ptr_current_dir_fcb->filename, sizeof("."), ".");
    strcpy_s(ptr_current_dir_fcb->extname, sizeof(""), "");
    ptr_current_dir_fcb->attribute = 0;
    ptr_current_dir_fcb->time.hour = now_time.tm_hour;
    ptr_current_dir_fcb->time.minute = now_time.tm_min;
    ptr_current_dir_fcb->time.second = now_time.tm_sec;
    ptr_current_dir_fcb->date.year = now_time.tm_year;
    ptr_current_dir_fcb->date.month = now_time.tm_mon;
    ptr_current_dir_fcb->date.day = now_time.tm_mday;
    ptr_current_dir_fcb->first = 5;
    ptr_current_dir_fcb->length = 2 * sizeof(fcb);
    ptr_current_dir_fcb->free = 1;

    fcb *ptr_father_dir_fcb = ptr_current_dir_fcb + 1;
    memcpy(ptr_father_dir_fcb, ptr_current_dir_fcb, sizeof(fcb));
    strcpy_s(ptr_father_dir_fcb->filename, sizeof(".."), "..");

    for (i = ROOT_BLOCK_NUM; i < (int) (BLOCK_SIZE / sizeof(fcb)); i++) {
        ptr_father_dir_fcb++;
        strcpy_s(ptr_father_dir_fcb->filename, sizeof(""), "");
        ptr_father_dir_fcb->free = 0;
    }
}

/**
 * 更改当前目录
 *
 * @param dirname   新的当前目录的目录名
 */
void my_cd(char *dirname) {
    if (ptr_current_dir->attribute == 1) {
        puts("Error(my_cd): 当前正在文件中，请使用 'close' 命令关闭文件");
        return;
    }

    int i, fd = -1, tag = -1;
    static char buffer[SIZE];
    ptr_current_dir->count = 0;
    do_read(ptr_current_dir - open_file_list, (int) ptr_current_dir->length, buffer);

    fcb *fcb_ptr = (fcb *) buffer;
    for (i = 0; i < (int) (ptr_current_dir->length / sizeof(fcb)); i++, fcb_ptr++) {
        if (strcmp(fcb_ptr->filename, dirname) == 0 && fcb_ptr->attribute == 0) {
            tag = 1;
            break;
        }
    }
    if (tag != 1) {
        puts("Error(my_cd): 错误，目标路径不存在");
        return;
    } else {
        if (strcmp(fcb_ptr->filename, ".") == 0) {
            return;
        } else if (strcmp(fcb_ptr->filename, "..") == 0) {
            if (ptr_current_dir - open_file_list == 0) {
                return;
            } else {
                my_close(ptr_current_dir - open_file_list);
                return;
            }
        } else {
            for (int j = 0; j < MAX_OPEN_FILE; j++) {
                if (open_file_list[j].t_openfile == 0) {
                    open_file_list[j].t_openfile = 1;
                    fd = j;
                    break;
                }
            }
            if (fd == -1) {
                puts("Error(my_cd): 已超过允许的最多同时打开个数");
                return;
            }
            open_file_list[fd].attribute = fcb_ptr->attribute;
            open_file_list[fd].count = 0;
            open_file_list[fd].date = fcb_ptr->date;
            open_file_list[fd].time = fcb_ptr->time;
            strcpy_s(open_file_list[fd].filename, sizeof(fcb_ptr->filename) + 1, fcb_ptr->filename);
            strcpy_s(open_file_list[fd].extname, sizeof(fcb_ptr->extname), fcb_ptr->extname);
            open_file_list[fd].first = fcb_ptr->first;
            open_file_list[fd].free = fcb_ptr->free;
            open_file_list[fd].fcb_state = 0;
            open_file_list[fd].length = fcb_ptr->length;
            strcat(strcat(strcpy((char *) open_file_list[fd].dir, (char *) (ptr_current_dir->dir)), dirname), "/");
            open_file_list[fd].t_openfile = 1;
            open_file_list[fd].dir_no = ptr_current_dir->first;
            open_file_list[fd].dir_off = i;
            ptr_current_dir = open_file_list + fd;
        }
    }

}

/**
 * 创建子目录
 *
 * @param dirname   新建目录的目录名
 */
void my_mkdir(char *dirname) {
    char *separator = ".", *tok_buffer, *filename = strtok_s(dirname, separator, &tok_buffer);
    static char buffer[SIZE];

    char *extname = strtok_s(NULL, separator, &tok_buffer);
    if (extname != NULL) {
        puts("Error(mkdir): 参数错误，目录项不允许使用扩展名");
        return;
    }

    ptr_current_dir->count = 0;
    int file_length = do_read(ptr_current_dir - open_file_list, (int) ptr_current_dir->length, buffer);
    fcb *fcb_ptr = (fcb *) buffer;

    for (int i = 0; i < (int) (file_length / sizeof(fcb)); i++) {
        if (strcmp(dirname, fcb_ptr[i].filename) == 0 && fcb_ptr->attribute == 0) {
            puts("Error(mkdir): 路径名已存在");
            return;
        }
    }

    int fd = -1;
    for (int i = 0; i < MAX_OPEN_FILE; i++) {
        if (open_file_list[i].t_openfile != 1) {
            open_file_list[i].t_openfile = 1;
            fd = i;
            break;
        }
    }
    if (fd == -1) {
        puts("Error(mkdir): 已超过允许的最多同时打开个数");
        return;
    }

    unsigned short int block_num = END;
    fat *fat_ptr = (fat *) (my_v_hard + BLOCK_SIZE);
    for (int i = 0; i < (int) (SIZE / BLOCK_SIZE); i++) {
        if (fat_ptr[i].id == FREE) {
            block_num = i;
            break;
        }
    }
    if (block_num == END) {
        puts("Error(mkdir): 磁盘块已满");
        return;
    }

    // 更新 FAT 表
    fat *fat1 = (fat *) (my_v_hard + BLOCK_SIZE), *fat2 = (fat *) (my_v_hard + BLOCK_SIZE * 3);
    fat1[block_num].id = END;
    fat2[block_num].id = END;

    int no;
    for (no = 0; no < (int) (file_length / sizeof(fcb)); no++) {
        if (fcb_ptr[no].free == 0) {
            break;
        }
    }

    ptr_current_dir->count = no * (int) sizeof(fcb);
    ptr_current_dir->fcb_state = 1;

    // 初始化 FCB
    time_t now;
    struct tm now_time;
    time(&now);
    localtime_s(&now_time, &now);

    fcb *new_fcb = (fcb *) malloc(sizeof(fcb));
    new_fcb->attribute = 0;
    new_fcb->time.hour = now_time.tm_hour;
    new_fcb->time.minute = now_time.tm_min;
    new_fcb->time.second = now_time.tm_sec;
    new_fcb->date.year = now_time.tm_year;
    new_fcb->date.month = now_time.tm_mon;
    new_fcb->date.day = now_time.tm_mday;
    strcpy_s(new_fcb->filename, strlen(dirname) + 1, dirname);
    strcpy_s(new_fcb->extname, sizeof(""), "");
    new_fcb->first = block_num;
    new_fcb->length = 2 * sizeof(fcb);
    new_fcb->free = 1;
    do_write(ptr_current_dir - open_file_list, (char *) new_fcb, sizeof(fcb), 2);

    // 设置打开文件表项
    open_file_list[fd].attribute = 0;
    open_file_list[fd].count = 0;
    open_file_list[fd].date = new_fcb->date;
    open_file_list[fd].time = new_fcb->time;
    open_file_list[fd].dir_no = ptr_current_dir->first;
    open_file_list[fd].dir_off = no;
    strcpy_s(open_file_list[fd].filename, strlen(dirname) + 1, dirname);
    strcpy_s(open_file_list[fd].extname, sizeof(""), "");
    open_file_list[fd].fcb_state = 0;
    open_file_list[fd].first = new_fcb->first;
    open_file_list[fd].free = new_fcb->free;
    open_file_list[fd].length = new_fcb->length;
    open_file_list[fd].t_openfile = 1;
    strcat(strcat(strcpy(open_file_list[fd].dir, (char *) (ptr_current_dir->dir)), dirname), "/");


    // 设置目录项
    fcb *ptr_current_dir_fcb = (fcb *) malloc(sizeof(fcb));
    ptr_current_dir_fcb->attribute = 0;
    ptr_current_dir_fcb->date = new_fcb->date;
    ptr_current_dir_fcb->time = new_fcb->time;
    strcpy_s(ptr_current_dir_fcb->filename, sizeof("."), ".");
    strcpy_s(ptr_current_dir_fcb->extname, sizeof(""), "");
    ptr_current_dir_fcb->first = block_num;
    ptr_current_dir_fcb->length = 2 * sizeof(fcb);
    ptr_current_dir_fcb->free = 1;
    do_write(fd, (char *) ptr_current_dir_fcb, sizeof(fcb), 2);

    fcb *ptr_father_dir_fcb = (fcb *) malloc(sizeof(fcb));
    memcpy(ptr_father_dir_fcb, ptr_current_dir_fcb, sizeof(fcb));
    strcpy_s(ptr_father_dir_fcb->filename, sizeof(".."), "..");
    ptr_father_dir_fcb->first = ptr_current_dir->first;
    ptr_father_dir_fcb->length = ptr_current_dir->length;
    ptr_father_dir_fcb->date = ptr_current_dir->date;
    ptr_father_dir_fcb->time = ptr_current_dir->time;
    ptr_father_dir_fcb->free = 1;
    do_write(fd, (char *) ptr_father_dir_fcb, sizeof(fcb), 2);

    my_close(fd);
    free(new_fcb);
    free(ptr_current_dir_fcb);
    free(ptr_father_dir_fcb);


    // 修改父目录 FCB
    fcb_ptr = (fcb *) buffer;
    fcb_ptr->length = ptr_current_dir->length;
    ptr_current_dir->count = 0;
    do_write(ptr_current_dir - open_file_list, (char *) fcb_ptr, sizeof(fcb), 2);
    ptr_current_dir->fcb_state = 1;
}

/**
 * 删除子目录
 *
 * @param dirname   欲删除目录的目录名
 */
void my_rmdir(char *dirname) {
    int i, tag = 0;
    static char buffer[SIZE];

    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0) {
        puts("Error(My_Rmdir): 非法操作，无法对 '.' 和 '..' 目录执行 'rmdir'");
        return;
    }
    ptr_current_dir->count = 0;
    do_read(ptr_current_dir - open_file_list, (int) ptr_current_dir->length, buffer);

    // 查找要删除的目录
    fcb *fcb_ptr = (fcb *) buffer;
    for (i = 0; i < (int) (ptr_current_dir->length / sizeof(fcb)); i++, fcb_ptr++) {
        if (fcb_ptr->free == 0)
            continue;
        if (strcmp(fcb_ptr->filename, dirname) == 0 && fcb_ptr->attribute == 0) {
            tag = 1;
            break;
        }
    }
    if (tag != 1) {
        puts("Error(My_Rmdir): 找不到该目录");
        return;
    }
    // 无法删除非空目录
    if (fcb_ptr->length > 2 * sizeof(fcb)) {
        puts("Error(My_Rmdir): 禁止对非空目录执行 'rmdir'");
        return;
    }

    // 更新 FAT 表
    int block_num = fcb_ptr->first;
    fat *fat1 = (fat *) (my_v_hard + BLOCK_SIZE);
    int nxt_num;
    while (1) {
        nxt_num = fat1[block_num].id;
        fat1[block_num].id = FREE;
        if (nxt_num != END) {
            block_num = nxt_num;
        } else {
            break;
        }
    }
    fat1 = (fat *) (my_v_hard + BLOCK_SIZE);
    fat *fat2 = (fat *) (my_v_hard + BLOCK_SIZE * 3);
    memcpy(fat2, fat1, BLOCK_SIZE * 2);

    // 更新 FCB
    fcb_ptr->date.day = 0;
    fcb_ptr->date.month = 0;
    fcb_ptr->date.year = 0;
    fcb_ptr->time.second = 0;
    fcb_ptr->time.minute = 0;
    fcb_ptr->time.hour = 0;
    fcb_ptr->extname[0] = '\0';
    fcb_ptr->filename[0] = '\0';
    fcb_ptr->first = 0;
    fcb_ptr->free = 0;
    fcb_ptr->length = 0;

    ptr_current_dir->count = (int) (i * sizeof(fcb));
    do_write(ptr_current_dir - open_file_list, (char *) fcb_ptr, sizeof(fcb), 2);

    // 考虑修改父目录 length， 循环删除 FCB
    int log_num = i;
    if ((log_num + 1) * sizeof(fcb) == ptr_current_dir->length) {
        ptr_current_dir->length -= sizeof(fcb);
        log_num--;
        fcb_ptr = (fcb *) buffer + log_num;
        while (fcb_ptr->free == 0) {
            fcb_ptr--;
            ptr_current_dir->length -= sizeof(fcb);
        }
    }

    // 更新父目录 FCB
    fcb_ptr = (fcb *) buffer;
    fcb_ptr->length = ptr_current_dir->length;
    ptr_current_dir->count = 0;
    do_write(ptr_current_dir - open_file_list, (char *) fcb_ptr, sizeof(fcb), 2);

    ptr_current_dir->fcb_state = 1;
}

/**
 * 显示目录
 */
void my_ls(void) {
    if (ptr_current_dir->attribute == 1) {
        puts("Error(ls): 非法操作，不能对文件执行 ls 操作");
        return;
    }
    char text[SIZE];

    puts("类型    名称              大小   时间");
    ptr_current_dir->count = 0;
    do_read(ptr_current_dir - open_file_list, (int) ptr_current_dir->length, text);

    fcb *fcb_ptr = (fcb *) text;
    for (int i = 0; i < (int) (ptr_current_dir->length / sizeof(fcb)); i++, fcb_ptr++) {
        if (fcb_ptr->free == 1) {
            if (fcb_ptr->attribute == 0) {
                printf("<DIR>   %-15s  -     %d/%02d/%02d %02d:%02d:%02d\n",
                       fcb_ptr->filename,
                       fcb_ptr->date.year + 1900,
                       fcb_ptr->date.month,
                       fcb_ptr->date.day,
                       fcb_ptr->time.hour,
                       fcb_ptr->time.minute,
                       fcb_ptr->time.second);
            } else if (fcb_ptr->attribute == 1) {
                printf("<--->   %-15s  %-4lu  %d/%02d/%02d %02d:%02d:%02d\n",
                       fcb_ptr->filename,
                       fcb_ptr->length,
                       fcb_ptr->date.year + 1900,
                       fcb_ptr->date.month,
                       fcb_ptr->date.day,
                       fcb_ptr->time.hour,
                       fcb_ptr->time.minute,
                       fcb_ptr->time.second);
            } else {
                puts("Error(ls): 文件系统错误，遇到未知类型");
                return;
            }
        }
    }

}

/**
 * 创建文件
 *
 * @param filename  新建文件的文件名，可能包含路径
 * @return          若创建成功，返回该文件的文件描述符（文件打开表中的数组下标）；否则返回 -1
 */
int my_create(char *filename) {
    if (ptr_current_dir->attribute == 1) {
        puts("Error(My_Create): 非法操作，不能在数据块中执行 'create'");
        return -1;
    }

    static char buffer[SIZE];

    ptr_current_dir->count = 0;
    do_read(ptr_current_dir - open_file_list, (int) ptr_current_dir->length, buffer);

    int i;
    fcb *fcb_ptr = (fcb *) buffer;
    // 检查重名
    for (i = 0; i < (int) (ptr_current_dir->length / sizeof(fcb)); i++, fcb_ptr++) {
        if (fcb_ptr->free == 0) {
            continue;
        }
        if (strcmp(fcb_ptr->filename, filename) == 0 && fcb_ptr->attribute == 1) {
            puts("Error(My_Create): 文件已经存在");
            return -1;
        }
    }

    // 申请空 FCB
    fcb_ptr = (fcb *) buffer;
    for (i = 0; i < (int) (ptr_current_dir->length / sizeof(fcb)); i++, fcb_ptr++) {
        if (fcb_ptr->free == 0)
            break;
    }
    // 申请磁盘块并更新 FAT 表
    int block_num = -1;
    fat *fat1 = (fat *) (my_v_hard + BLOCK_SIZE);
    for (int j = 0; j < (int) (SIZE / BLOCK_SIZE); j++) {
        if (fat1[j].id == FREE) {
            block_num = j;
            break;
        }
    }
    if (block_num == -1) {
        return -1;
    }
    fat *fat11 = (fat *) (my_v_hard + BLOCK_SIZE);
    fat *fat2 = (fat *) (my_v_hard + BLOCK_SIZE * 3);
    fat11[block_num].id = END;
    memcpy(fat2, fat11, BLOCK_SIZE * 2);

    // 修改 FCB 信息
    strcpy_s(fcb_ptr->filename, strlen(filename) + 1, filename);
    time_t now;
    struct tm now_time;
    time(&now);
    localtime_s(&now_time, &now);
    fcb_ptr->date.year = now_time.tm_year;
    fcb_ptr->date.month = now_time.tm_mon;
    fcb_ptr->date.day = now_time.tm_mday;
    fcb_ptr->time.hour = now_time.tm_hour;
    fcb_ptr->time.minute = now_time.tm_min;
    fcb_ptr->time.second = now_time.tm_sec;
    fcb_ptr->first = block_num;
    fcb_ptr->free = 1;
    fcb_ptr->attribute = 1;
    fcb_ptr->length = 0;

    ptr_current_dir->count = (int) (i * sizeof(fcb));
    do_write(ptr_current_dir - open_file_list, (char *) fcb_ptr, sizeof(fcb), 2);

    // 修改父目录 FCB
    fcb_ptr = (fcb *) buffer;
    fcb_ptr->length = ptr_current_dir->length;
    ptr_current_dir->count = 0;
    do_write(ptr_current_dir - open_file_list, (char *) fcb_ptr, sizeof(fcb), 2);

    ptr_current_dir->fcb_state = 1;
}

/**
 * 删除文件
 *
 * @param filename 欲删除文件的文件名，可能还包含路径
 */
void my_rm(char *filename) {
    static char buffer[SIZE];
    ptr_current_dir->count = 0;
    do_read(ptr_current_dir - open_file_list, (int) ptr_current_dir->length, buffer);

    int i, flag = 0;
    fcb *fcb_ptr = (fcb *) buffer;

    for (i = 0; i < ((int) ptr_current_dir->length / sizeof(fcb)); i++, fcb_ptr++) {
        if (strcmp(fcb_ptr->filename, filename) == 0 && fcb_ptr->attribute == 1) {
            flag = 1;
            break;
        }
    }
    if (flag != 1) {
        puts("Error(my_rm): 无此文件");
        return;
    }

    // 更新 FAT 表
    int block_num = fcb_ptr->first;
    fat *fat1 = (fat *) (my_v_hard + BLOCK_SIZE);
    int nxt_num = 0;
    while (TRUE) {
        nxt_num = fat1[block_num].id;
        fat1[block_num].id = FREE;
        if (nxt_num != END)
            block_num = nxt_num;
        else
            break;
    }
    fat1 = (fat *) (my_v_hard + BLOCK_SIZE);
    fat *fat2 = (fat *) (my_v_hard + BLOCK_SIZE * 3);
    memcpy(fat2, fat1, BLOCK_SIZE * 2);

    // 清空 FCB
    fcb_ptr->date.day = 0;
    fcb_ptr->date.month = 0;
    fcb_ptr->date.year = 0;
    fcb_ptr->time.second = 0;
    fcb_ptr->time.minute = 0;
    fcb_ptr->time.hour = 0;
    fcb_ptr->extname[0] = '\0';
    fcb_ptr->filename[0] = '\0';
    fcb_ptr->first = 0;
    fcb_ptr->free = 0;
    fcb_ptr->length = 0;
    ptr_current_dir->count = (int) (i * sizeof(fcb));
    do_write(ptr_current_dir - open_file_list, (char *) fcb_ptr, sizeof(fcb), 2);
    //
    int log_num = i;
    if ((log_num + 1) * sizeof(fcb) == ptr_current_dir->length) {
        ptr_current_dir->length -= sizeof(fcb);
        log_num--;
        fcb_ptr = (fcb *) buffer + log_num;
        while (fcb_ptr->free == 0) {
            fcb_ptr--;
            ptr_current_dir->length -= sizeof(fcb);
        }
    }

    // 修改父目录 . 目录文件的 FCB
    fcb_ptr = (fcb *) buffer;
    fcb_ptr->length = ptr_current_dir->length;
    ptr_current_dir->count = 0;
    do_write(ptr_current_dir - open_file_list, (char *) fcb_ptr, sizeof(fcb), 2);

    ptr_current_dir->fcb_state = 1;

}

/**
 * 打开文件
 *
 * @param filename  欲打开文件的文件名
 * @return          若打开成功，返回该文件的描述符（在用户打开文件表中表项序号）；否则返回 -1
 */
int my_open(char *filename) {
    static char buffer[SIZE];
    ptr_current_dir->count = 0;
    do_read(ptr_current_dir - open_file_list, (int) ptr_current_dir->length, buffer);

    int i, flag = 0;
    fcb *fcb_ptr = (fcb *) buffer;

    for (i = 0; i < (int) (ptr_current_dir->length / sizeof(fcb)); i++, fcb_ptr++) {
        if (strcmp(fcb_ptr->filename, filename) == 0 && fcb_ptr->attribute == 1) {
            flag = 1;
            break;
        }
    }
    if (flag != 1) {
        puts("Error(My_Open): 文件不存在");
        return -1;
    }

    int fd = -1;
    for (int j = 0; j < MAX_OPEN_FILE; j++) {
        if (open_file_list[j].t_openfile == 0) {
            open_file_list[j].t_openfile = 1;
            fd = j;
            break;
        }
    }
    if (fd == -1) {
        puts("Error(My_Open): 已超过允许的最多同时打开个数");
        return -1;
    }

    open_file_list[fd].attribute = 1;
    open_file_list[fd].count = 0;
    open_file_list[fd].date = fcb_ptr->date;
    open_file_list[fd].time = fcb_ptr->time;
    open_file_list[fd].length = fcb_ptr->length;
    open_file_list[fd].first = fcb_ptr->first;
    open_file_list[fd].free = 1;
    strcpy_s(open_file_list[fd].filename, strlen(fcb_ptr->filename) + 1, fcb_ptr->filename);
    strcat(strcpy((char *) open_file_list[fd].dir, (char *) (ptr_current_dir->dir)), filename);
    open_file_list[fd].dir_no = ptr_current_dir->first;
    open_file_list[fd].dir_off = i;
    open_file_list[fd].t_openfile = 1;

    open_file_list[fd].fcb_state = 0;

    ptr_current_dir = open_file_list + fd;
    return fd;
}

/**
 * 关闭文件
 *
 * @param fd 文件描述符
 */
void my_close(int fd) {
    if (fd > MAX_OPEN_FILE || fd < 0) {
        puts("Error(My_Close): 非法操作，文件描述符越界");
        return;
    }

    int i, father_fd = -1;
    fcb *fcb_ptr;

    for (i = 0; i < MAX_OPEN_FILE; i++) {
        if (open_file_list[i].first == open_file_list[fd].dir_no) {
            father_fd = i;
            break;
        }
    }
    if (father_fd == -1) {
        puts("Error(My_Close): 文件系统错误，父目录缺失");
        return;
    }

    static char buffer[SIZE];
    if (open_file_list[fd].fcb_state == 1) {
        do_read(father_fd, (int) open_file_list[father_fd].length, buffer);

        fcb_ptr = (fcb *) (buffer + sizeof(fcb) * open_file_list[fd].dir_off);
        strcpy_s(fcb_ptr->extname, sizeof(open_file_list[fd].extname), open_file_list[fd].extname);
        strcpy_s(fcb_ptr->filename, sizeof(open_file_list[fd].filename), open_file_list[fd].filename);
        fcb_ptr->first = open_file_list[fd].first;
        fcb_ptr->free = open_file_list[fd].free;
        fcb_ptr->length = open_file_list[fd].length;
        fcb_ptr->time = open_file_list[fd].time;
        fcb_ptr->date = open_file_list[fd].date;
        fcb_ptr->attribute = open_file_list[fd].attribute;
        open_file_list[father_fd].count = open_file_list[fd].dir_off * (int) sizeof(fcb);

        do_write(father_fd, (char *) fcb_ptr, sizeof(fcb), 2);
    }

    // 释放打开文件表
    memset(&open_file_list[fd], 0, sizeof(user_open));
    ptr_current_dir = open_file_list + father_fd;
}

/**
 * 写文件
 *
 * @param fd        open() 函数的返回值，文件的描述符
 * @return          实际写入的字节数
 */
int my_write(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILE) {
        puts("Error(My_Write): 文件不存在");
        return -1;
    }

    int w_style;
    while (1) {
        puts("请选择写方式：");
        puts("1. 截断写: 清空全部内容，从头开始");
        puts("2. 覆盖写: 从文件指针处开始覆盖写入");
        puts("3. 追加写: 追加写入");
        scanf_s("%d", &w_style);
        if (w_style > 3 || w_style < 1) {
            printf("input error\n");
        } else {
            break;
        }
    }
    general_buffer[0] = '\0';
    char text_tmp[SIZE] = "\0";
    puts("Info(My_Write): 请输入，换行后输入 ':wq!' 结束输入");
    getchar();

    while (TRUE) {
        fgets(text_tmp, SIZE, stdin);
        if (strcmp(text_tmp, ":wq!\n") == 0) {
            break;
        }
        strcat_s(general_buffer, sizeof(general_buffer), text_tmp);
    }

    general_buffer[strlen(general_buffer)] = '\0';
    do_write(fd, general_buffer, (int) strlen(general_buffer) + 1, (char) w_style);
    open_file_list[fd].fcb_state = 1;

    return (int) strlen(general_buffer) + 1;
}

/**
 * 写文件动作
 *
 * @param fd        open() 函数的返回值，文件的描述符
 * @param text      指向要写入的内容的指针
 * @param len       本次要求写入字节数
 * @param w_style   写方式: 1-截断写 2-覆盖写 3-追加写
 * @return          实际写入的字节数
 */
int do_write(int fd, char *text, int len, char w_style) {
    unsigned char *buffer = (unsigned char *) malloc(BLOCK_SIZE);
    if (buffer == NULL) {
        puts("Error(Do_Write): 缓冲区空间申请失败");
        return -1;
    }

    int block_num = open_file_list[fd].first, read_count = 0;
    char *text_ptr = text;
    fat *fat_ptr = (fat *) (my_v_hard + BLOCK_SIZE) + block_num;

    if (w_style == 1) {
        open_file_list[fd].count = 0;
        open_file_list[fd].length = 0;
    } else if (w_style == 3) {
        open_file_list[fd].count = (int) open_file_list[fd].length;
        if (open_file_list[fd].attribute == 1) {
            // 非空文件删除末尾 \0
            if (open_file_list[fd].length != 0) {
                open_file_list[fd].count = (int) open_file_list[fd].length - 1;
            }
        }
    }
    int offset = open_file_list[fd].count;

    // 定位磁盘块和块内偏移量
    while (offset >= BLOCK_SIZE) {
        block_num = fat_ptr->id;
        if (block_num == END) {
            puts("Error(Do_Write): 当前用户打开文件表的读写指针溢出");
            return -1;
        }
        fat_ptr = (fat *) (my_v_hard + BLOCK_SIZE) + block_num;
        offset -= BLOCK_SIZE;
    }

    unsigned char *block_ptr = (unsigned char *) (my_v_hard + block_num * BLOCK_SIZE);
    while (len > read_count) {
        memcpy(buffer, block_ptr, BLOCK_SIZE);
        for (; offset < BLOCK_SIZE; offset++) {
            *(buffer + offset) = *text_ptr;
            text_ptr++;
            read_count++;
            if (len == read_count) {
                break;
            }
        }
        memcpy(block_ptr, buffer, BLOCK_SIZE);

        // 向下一个磁盘块写入，如果没有磁盘块则进行申请
        if (offset == BLOCK_SIZE && len != read_count) {
            offset = 0;
            block_num = fat_ptr->id;
            if (block_num == END) {
                fat *new_fat = (fat *) (my_v_hard + BLOCK_SIZE);
                for (int j = 0; j < (int) (SIZE / BLOCK_SIZE); j++) {
                    if (new_fat[j].id == FREE) {
                        block_num = j;
                    }
                }
                if (block_num == END) {
                    puts("Error(Do_Write): 磁盘块分配失败，空间已满");
                    return -1;
                }
                block_ptr = (unsigned char *) (my_v_hard + BLOCK_SIZE * block_num);
                fat_ptr->id = block_num;
                fat_ptr = (fat *) (my_v_hard + BLOCK_SIZE) + block_num;
                fat_ptr->id = END;
            } else {
                block_ptr = (unsigned char *) (my_v_hard + BLOCK_SIZE * block_num);
                fat_ptr = (fat *) (my_v_hard + BLOCK_SIZE) + block_num;
            }
        }
    }

    open_file_list[fd].count += len;
    if (open_file_list[fd].count > open_file_list[fd].length) {
        open_file_list[fd].length = open_file_list[fd].count;
    }

    // 定位磁盘块和块内偏移量
    if (w_style == 1 || (w_style == 2 && open_file_list[fd].attribute == 0)) {
        int i;
        offset = (int) open_file_list[fd].length;
        fat_ptr = (fat *) (my_v_hard + BLOCK_SIZE) + open_file_list[fd].first;

        while (offset >= BLOCK_SIZE) {
            block_num = fat_ptr->id;
            offset -= BLOCK_SIZE;
            fat_ptr = (fat *) (my_v_hard + BLOCK_SIZE) + block_num;
        }
        while (TRUE) {
            if (fat_ptr->id != END) {
                i = fat_ptr->id;
                fat_ptr->id = FREE;
                fat_ptr = (fat *) (my_v_hard + BLOCK_SIZE) + i;
            } else {
                fat_ptr->id = FREE;
                break;
            }
        }

        fat_ptr = (fat *) (my_v_hard + BLOCK_SIZE) + block_num;
        fat_ptr->id = END;
    }

    memcpy((fat *) (my_v_hard + BLOCK_SIZE * 3), (fat *) (my_v_hard + BLOCK_SIZE), BLOCK_SIZE * 2);
    return len;
}

/**
 * 读文件
 *
 * @param fd        open() 函数的返回值，文件的描述符
 * @param len       要从文件中读出的字节数
 * @return          实际读出的字节数
 */
int my_read(int fd, int len) {
    if (fd < 0 || fd >= MAX_OPEN_FILE) {
        puts("Error(My_Read): 文件不存在");
        return -1;
    }

    static char buffer[SIZE];
    ptr_current_dir->count = 0;
    int length = do_read(fd, len, buffer);
    puts(buffer);
    return length;
}

/**
 * 读文件动作
 *
 * @param fd        open() 函数的返回值，文件的描述符
 * @param len       要求从文件中读出的字节数
 * @param text      指向存放读出数据的用户区地址
 * @return          实际读出的字节数
 */
int do_read(int fd, int len, char *text) {
    unsigned char *buffer = (unsigned char *) malloc(BLOCK_SIZE);
    if (buffer == NULL) {
        puts("Error(Do_Read): 缓冲区空间申请失败");
        return -1;
    }

    int required_len = len;
    char *text_ptr = text;
    int block_num = open_file_list[fd].first;
    int offset = open_file_list[fd].count;
    fat *fat_ptr = (fat *) (my_v_hard + BLOCK_SIZE) + block_num;

    // 定位读取目标磁盘块和块内地址
    while (offset >= BLOCK_SIZE) {
        offset -= BLOCK_SIZE;
        block_num = fat_ptr->id;
        if (block_num == END) {
            puts("Error(Do_Read): 目标磁盘块不存在");
            return -1;
        }
        fat_ptr = (fat *) (my_v_hard + BLOCK_SIZE) + block_num;
    }

    unsigned char *block_ptr = my_v_hard + block_num * BLOCK_SIZE;
    memcpy(buffer, block_ptr, BLOCK_SIZE);

    while (len > 0) {
        if (BLOCK_SIZE - offset > len) {
            memcpy(text_ptr, buffer + offset, len);
            text_ptr += len;
            offset += len;
            open_file_list[fd].count += len;
            len = 0;
        } else {
            memcpy(text_ptr, buffer + offset, BLOCK_SIZE - offset);
            text_ptr += BLOCK_SIZE - offset;
            len -= BLOCK_SIZE - offset;

            block_num = fat_ptr->id;
            if (block_num == END) {
                break;
            }
            fat_ptr = (fat *) (my_v_hard + BLOCK_SIZE) + block_num;
            block_ptr = my_v_hard + BLOCK_SIZE * block_num;
            memcpy(buffer, block_ptr, BLOCK_SIZE);
        }
    }

    free(buffer);
    return required_len - len;
}

/**
 * 退出文件系统
 */
void my_exit_sys() {
    while (ptr_current_dir - open_file_list) {
        my_close(ptr_current_dir - open_file_list);
    }

    if (fopen_s(&fptr, SYS_FILE, "w") != 0) {
        puts("Error(My_Exit): 文件指针操作失败");
        return;
    }
    fwrite(my_v_hard, SIZE, 1, fptr);
    fclose(fptr);
    puts("Info(My_Exit): 已保存并退出文件系统");
}

/**
 * 文件系统初始化
 */
void init_file_system() {
    if (fopen_s(&fptr, SYS_FILE, "w") != 0) {
        puts("Error(My_Format): 文件指针操作失败");
        return;
    }
    puts("Error(My_Format): myfsys 文件系统不存在，现在开始创建文件系统");
    my_format();
}

/**
 * 命令行初始化
 */
void init_cmd() {
    puts("------------------------------------命令行操作指南------------------------------------");
    puts("mkdir：创建子目录  rmdir：删除子目录  ls：显示目录中的内容  cd：更改当前目录  create：创建文件");
    puts("open：打开文件  close：关闭文件  write：写文件  read：读文件  rm：删除文件  exit：退出文件系统");
    puts("---------------------!危险操作!     format：格式化磁盘     !危险操作!--------------------");
    puts("------------------------------------------------------------------------------------");
}