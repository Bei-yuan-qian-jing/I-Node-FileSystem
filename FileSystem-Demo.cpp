#include <math.h>
#include <string.h>
#include <ctime>
#include <iomanip>
#include <iostream>
#include "ModelDefinition.h"
using namespace std;

int findFileINode(long p, char name[]) {
    int pointer = -1;
    time_t ti;
    for (int i = 0; i < 50; i++) {
        if (strcmp(disk[p].Dire_Block->directory[i].name, name) == 0) {
            pointer = disk[p].Dire_Block->directory[i].inode_number;
            time(&ti);
            disk[pointer].i_node->access_time = ti;
            return pointer;
        }
    }
    return pointer;
}

int findFreeINode() {
    int freeINodeIndex = 0;
    for (int i = 0; i < disk[1].super_block->total_data_block; i++)
        if (disk[2].i_node_bit_map->inode_bit_map[i] == false) {
            freeINodeIndex = i + disk[1].super_block->first_inode_block;
            break;
        }
    return freeINodeIndex;
}

int findFreeDataBlock() {
    int freeDataBlockIndex = 0;
    for (int i = 0; i < disk[1].super_block->total_data_block; i++)
        if (disk[3].data_bit_map->data_bit_map[i] == false) {
            freeDataBlockIndex = i + SYSTEM_USED;
            break;
        }
    return freeDataBlockIndex;
}

DIRECTORY_BLOCK* newDirectory() {
    DIRECTORY_BLOCK* temp = new DIRECTORY_BLOCK;
    for (int i = 0; i < 50; i++)
        memset(temp->directory[i].name, 0, 16);
    return temp;
}

INDIRECT_ADDR_BLOCK* newINDIR_Addr() {
    INDIRECT_ADDR_BLOCK* temp = new INDIRECT_ADDR_BLOCK;
    for (int i = 0; i < 256; i++)
        temp->addr[i] = 0;
    return temp;
}

void assign_INode(int I) {
    time_t t;
    time(&t);
    disk[I].i_node->create_time = disk[I].i_node->access_time = t;
    disk[I].i_node->current_size = 0;
    for (int i = 0; i < 10; i++) {
        disk[I].i_node->direct_addr[i] = 0;
    }
    for (int i = 0; i < 2; i++) {
        disk[I].i_node->indirect_addr[i] = 0;
    }
}

void make_Dir(long& p, char* token) {
    for (int i = 0; i < 50; i++) {
        if (strlen(disk[p].Dire_Block->directory[i].name) == 0) {
            strcpy(disk[p].Dire_Block->directory[i].name, token);
            int Inode = findFreeINode();
            disk[p].Dire_Block->directory[i].inode_number = Inode;
            assign_INode(Inode);  // request inode for new directory
            disk[Inode].i_node->type = I_NODE_TYPE_DIR;
            disk[2].i_node_bit_map->inode_bit_map[Inode - disk[1].super_block->first_inode_block] = true;
            int data_p = findFreeDataBlock();  // request data block for new directory
            disk[1].super_block->free_data_block--;
            disk[Inode].i_node->direct_addr[disk[Inode].i_node->current_size++] = data_p;
            disk[data_p].Dire_Block = newDirectory();
            disk[3].data_bit_map->data_bit_map[data_p - SYSTEM_USED] = true;
            strcpy(disk[data_p].Dire_Block->name, token);

            p = data_p;
            break;
        }
    }
}

void make_inDir(INDIRECT_ADDR_BLOCK* p, char* filecontent, int& totalBlocks, int& used_Blocks) {
    int count = totalBlocks < 256 ? totalBlocks : 256;
    int temp;
    for (int i = 0; i < count; i++) {
        temp = p->addr[i] = findFreeDataBlock();
        disk[1].super_block->free_data_block--;
        disk[3].data_bit_map->data_bit_map[temp - SYSTEM_USED] = true;
        disk[temp].data = new DATA_BLOCK;
        memset(disk[temp].data->content, 0, 1024);
        totalBlocks--;
        if (used_Blocks > 0) {
            // copy the file content into the new space;
            used_Blocks--;
            strncpy(disk[temp].data->content, filecontent, 1024);
            filecontent += 1024;
        }
    }
}

void make_File(int& p, char* fileName, double size, char* filecontent) {
    int iNodeIndex;
    for (int i = 0; i < MAX_FILE_NUM_IN_DIR; i++) {
        // 找到当前文件夹下第一个为空的文件名（即第一个未被其他文件占用的位置），并将文件保存在这里
        if (strlen(disk[p].Dire_Block->directory[i].name) == 0) {
            strcpy(disk[p].Dire_Block->directory[i].name, fileName);
            iNodeIndex = findFreeINode();
            if (iNodeIndex == -1) {
                //返回的I-Node不可用
                return;
            }
            // 将此文件的I-Node编号设为返回的free I-Node编号
            disk[p].Dire_Block->directory[i].inode_number = iNodeIndex;
            assign_INode(iNodeIndex);  // allowcate inode for new directory
            disk[iNodeIndex].i_node->type = I_NODE_TYPE_FILE;
            int iNodeBitMapOffset = iNodeIndex - disk[1].super_block->first_inode_block;
            disk[2].i_node_bit_map->inode_bit_map[iNodeBitMapOffset] = true;
            p = iNodeIndex;
            break;
        }
    }
    // 文件目前占用的空间
    int usedSpace = disk[p].i_node->current_size = strlen(filecontent);
    disk[p].i_node->max_size = size;

    // 文件目前需要占用的block数
    int used_Blocks = (int)ceil((double)usedSpace / 1024);
    // 文件最多可以占用的block数(即文件达到maxSize时所占用的block数)
    int totalBlocks = (int)ceil(size / 1024);
    int used_Blocks2 = used_Blocks;

    int count = totalBlocks < 10 ? totalBlocks : 10; //分配到direct address的block数量
    int temp;
    for (int i = 0; i < count; i++) {
        temp = disk[iNodeIndex].i_node->direct_addr[i] = findFreeDataBlock();
        int dataBitMapOffset = temp - SYSTEM_USED;
        disk[3].data_bit_map->data_bit_map[dataBitMapOffset] = true;
        disk[temp].data = new DATA_BLOCK;
        memset(disk[temp].data->content, 0, 1024);
        disk[1].super_block->free_data_block--;
        totalBlocks--;
        if (used_Blocks > 0) {
            used_Blocks--;
            // 从fileContent中拷贝一个字节的字符串到data_block中
            strncpy(disk[temp].data->content, filecontent, 1024);
            filecontent += 1024;
        }
    }

    if (totalBlocks > 0)  //第一级indirect address
    {
        int new_p = findFreeDataBlock();  // request data block for new data;
        disk[new_p].in_addr = newINDIR_Addr();
        disk[iNodeIndex].i_node->indirect_addr[0] = new_p;
        disk[3].data_bit_map->data_bit_map[new_p - SYSTEM_USED] = true;
        make_inDir(disk[new_p].in_addr, filecontent, totalBlocks, used_Blocks);
    }
    if (totalBlocks > 0)  //第二级indirect address
    {
        int new_p1 = findFreeDataBlock();  // request data block for new data;
        disk[new_p1].in_addr = newINDIR_Addr();
        disk[iNodeIndex].i_node->indirect_addr[1] = new_p1;
        disk[3].data_bit_map->data_bit_map[new_p1 - SYSTEM_USED] = true;
        int count = 0;
        while (totalBlocks > 0 && count < 256) {
            int new_p2 = findFreeDataBlock();  // request data block for new data;
            disk[new_p2].in_addr = newINDIR_Addr();
            disk[new_p1].in_addr->addr[count++] = new_p2;
            disk[3].data_bit_map->data_bit_map[new_p2 - SYSTEM_USED] = true;
            make_inDir(disk[new_p2].in_addr, filecontent, totalBlocks,used_Blocks);
        }
    }
    filecontent -= used_Blocks2 * 1024;
    delete []filecontent;
}

bool deleteFileHelp(int& p, int i) {
    int inode;
    inode = disk[p].Dire_Block->directory[i].inode_number;  // get the I_node of the file

    strcpy(disk[p].Dire_Block->directory[i].name, "");
    disk[p].Dire_Block->directory[i].inode_number = 0;  //在目录上清除

    int tp;  // delete data
    for (int i = 0; i < 10; i++) {
        tp = (int)disk[inode].i_node->direct_addr[i];
        if (tp == 0)
            break;
        delete disk[tp].data;
        disk[3].data_bit_map->data_bit_map[tp - SYSTEM_USED] = false;
        disk[1].super_block->free_data_block++;
    }

    tp = (int)disk[inode].i_node->indirect_addr[0];
    int tp1;
    if (tp != 0) {
        for (int i = 0; i < 256; i++) {
            tp1 = (int)disk[tp].in_addr->addr;
            if (tp1 == 0)
                break;
            delete disk[tp1].data;
            disk[3].data_bit_map->data_bit_map[tp1 - SYSTEM_USED] = false;
            disk[1].super_block->free_data_block++;
        }
    }

    tp = (int)disk[inode].i_node->indirect_addr[1];
    if (tp != 0) {
        int tp2;
        for (int i = 0; i < 256; i++) {
            tp1 = disk[tp].in_addr->addr[i];
            if (tp1 == 0)
                break;
            for (int j = 0; j < 256; j++) {
                tp2 = disk[tp1].in_addr->addr[j];
                if (tp2 == 0)
                    break;
                delete disk[tp2].data;
                disk[3].data_bit_map->data_bit_map[tp2 - SYSTEM_USED] = false;
                disk[1].super_block->free_data_block++;
            }
        }
    }
    disk[2]
        .i_node_bit_map
        ->inode_bit_map[inode - disk[1].super_block->first_inode_block] = false;
    delete disk[inode].i_node;
    disk[inode].i_node = new I_NODE;
    return true;
}

//返回文件 的Inode
int searchFile(char* filename) {
    int pointer = -1;
    char* token = strtok(filename, "/");
    long p = disk[1].super_block->first_data_block;
    token = strtok(NULL, "/");
    int tp;
    while (token != NULL) {
        if ((tp = findFileINode(p, token)) == -1)
            return pointer;
        p = disk[tp].i_node->direct_addr[0];
        token = strtok(NULL, "/");
    }
    pointer = tp;
    return pointer;
}

//返回目录文件的地址
int searchDir(char* dir) {
    long pointer = -1;
    char* sub = strtok(dir, "/");
    long p = disk[1].super_block->first_data_block;
    sub = strtok(NULL, "/");
    int tp;
    while (sub != NULL) {
        if ((tp = findFileINode(p, sub)) == -1)
            return pointer;
        p = disk[tp].i_node->direct_addr[0];  //获取下一级目录的地址

        sub = strtok(NULL, "/");
    }
    pointer = p;
    return pointer;
}

void fixDirPath(char* dir) {
    if (strchr(dir, '/') == NULL) {
        char temp[1024];
        strcpy(temp, dir);
        strcpy(dir, disk[1].super_block->cwdir);
        strcat(dir, "/");
        strcat(dir, temp);
    }
}

void showhelp() {
    cout << "\thelp\t\tshow help information\t\t\t\tinstruction format" << endl;
    cout << "\tcreateFile\tcreate a new file\t\t\t\tcreateFile filePath fileSize" << endl;
    cout << "\tdeleteFile\tdelete a file\t\t\t\t\tdeleteFile filePath" << endl;
    cout << "\tcreateDir\tcreate a new directory\t\t\t\tcreateDir /dir1/sub1" << endl;
    cout << "\tdeleteDir\tdelete a directory\t\t\t\tdeleteDir /dir1/sub1" << endl;
    cout << "\tchangeDir\tchange current working direcotry\t\tchangeDir /dir2" << endl;
    cout << "\tdir\t\tlist all the file and sub directory information\tdir dirPath" << endl;
    cout << "\tcp\t\tcopy a file\t\t\t\t\tcp file1 file2" << endl;
    cout << "\tsum\t\tdisplay the usage of storage space" << endl;
    cout << "\tcat\t\tprint out the file contents\t\t\tcat /dir1/file1" << endl;
    cout << "\texit\t\texit the fileSystem" << endl;
};

void createRoot() {
    int p = disk[1].super_block->first_data_block;
    disk[p].Dire_Block = newDirectory();
    disk[1].super_block->free_data_block--;
    disk[3].data_bit_map->data_bit_map[0] = true;
    strcpy(disk[p].Dire_Block->name, "root");
    strcpy(disk[1].super_block->usrdir, disk[p].Dire_Block->name);
}

void systemBoot() {
    disk = new DISK[16 * 1024];
    
    // 初始化super_block
    disk[1].super_block = new SUPER_BLOCK;
    disk[1].super_block->free_inode = 1638;
    disk[1].super_block->free_data_block = 16194;
    disk[1].super_block->total_data_block = 16194;
    disk[1].super_block->sizeof_block = 1024;
    disk[1].super_block->inode_bitmap_block = 2;
    disk[1].super_block->data_bitmap_block = 3;
    disk[1].super_block->first_inode_block = 4;
    disk[1].super_block->last_inode_block = 1641;
    disk[1].super_block->first_data_block = 1642;
    strcpy(disk[1].super_block->cwdir, "/root");
    strcpy(disk[1].super_block->usrdir, "/root");

    // 初始化i_node_bit_map
    disk[2].i_node_bit_map = new INODE_BIT_MAP;
    memset(disk[2].i_node_bit_map->inode_bit_map, 0, sizeof(disk[2].i_node_bit_map->inode_bit_map));

    // 初始化data_bit_map
    disk[3].data_bit_map = new DATA_BIT_MAP;
    memset(disk[3].data_bit_map->data_bit_map, 0, sizeof(disk[3].data_bit_map->data_bit_map));

    // 初始化i_node
    for (int i = 4; i < SYSTEM_USED; i++) {
        disk[i].i_node = new I_NODE;
    }

    // 初始化i_node_bit_map
    disk[1].i_node_bit_map = new INODE_BIT_MAP;

    //程序启动创建根文件夹。
    createRoot(); 
}

int createDirectory(char dir[]) {
    int pointer = -1;
    //如果是在当前用户的目录下创建新路径，则请求被允许
    if (strstr(dir, disk[1].super_block->usrdir) !=  NULL) {
        char* token = strtok(dir, "/");
        long p = disk[1].super_block->first_data_block;
        token = strtok(NULL, "/");
        int tp;
        while (token != NULL) {
            if ((tp = findFileINode(p, token)) ==
                -1)  //看看是否找到与token对应的inode
            {
                make_Dir(p, token);  //如果没有找到则在该级路径想创建新的文件夹
                time_t ti;
                time(&ti);
            } else  //如果已经找到
            {
                p = disk[tp].i_node->direct_addr[0];  //通过inode访问下一级路径
            }
            token = strtok(NULL, "/");
        };
        pointer = p;
    } else
        cout << "Permission denied!" << endl;
    return pointer;
}

void createFile(char* filePath, int size, char* filecontent) {
    // 若当前文件路径不包含用户目录（即所创建的文件必须要在/root/下），则无法创建
    if (strstr(filePath, disk[1].super_block->usrdir) == NULL) {
        cout << "Permission denied" << endl;
        return;
    }
    // strrchr() 函数查找字符串在另一个字符串中最后一次出现的位置，并返回从该位置到字符串结尾的所有字符
    char* filename = strrchr(filePath, '/');
    // 剔除文件名最开始的'/'
    filename++;     

    char dir[1024];
    memset(dir, 0, sizeof(dir));
    // 将指定长的字符串从filePath复制到dir，从而获取目录地址
    strncpy(dir, filePath, strlen(filePath) - strlen(filename) - 1);                                       

    int pointer;
    // 尝试创建文件夹
    if ((pointer = createDirectory(dir)) == -1) {
        // 创建文件夹失败
        cout << "Directory creating denied" << endl;
    } else {
        for (int i = 0; i < MAX_FILE_NUM_IN_DIR; i++)
            if (strcmp(disk[pointer].Dire_Block->directory[i].name, filename) == 0) {
                cout << "Same file name in the directory，would you like to replace it<y/n>" << endl;
                char n;
                cin >> n;
                if (n == 'n')
                    return;
                else if (!deleteFileHelp(pointer, i)) {
                    // 删除文件失败
                    return;
                }
            }
        make_File(pointer, filename, size, filecontent);
    }
}

void cat(char* filename) {
    int p;
    if (strstr(filename, disk[1].super_block->usrdir) == NULL) {
        cout << "Permission denied" << endl;
        return;
    }
    if ((p = searchFile(filename)) == -1) {
        cout << "file doesn't exist in this directory!" << endl;
        return;
    }
    int tp;
    for (int i = 0; i < 10; i++) {
        tp = (int)disk[p].i_node->direct_addr[i];
        if (tp == 0)
            break;
        cout << disk[tp].data->content;
    }
    tp = (int)disk[p].i_node->indirect_addr[0];
    int tp1;
    if (tp != 0) {
        for (int i = 0; i < 256; i++) {
            tp1 = (int)disk[tp].in_addr->addr[i];
            if (tp1 == 0)
                break;
            cout << disk[tp1].data->content;
        }
    }
    tp = (int)disk[p].i_node->indirect_addr[1];
    int tp2;
    if (tp != 0) {
        for (int i = 0; i < 256; i++) {
            tp1 = (int)disk[tp].in_addr->addr[i];
            if (tp1 == 0)
                break;
            for (int j = 0; j < 256; j++) {
                tp2 = (int)disk[tp1].in_addr->addr[i];
                if (tp2 == 0)
                    break;
                cout << disk[tp2].data->content;
            }
        }
    }
    cout << endl;
}

void deleteFile(char* name) {
    if (strstr(name, disk[1].super_block->usrdir) == NULL) {
        cout << "Permission denied" << endl;
        return;
    }
    char* filename = strrchr(name, '/');
    char dir[16];
    memset(dir, 0, sizeof(dir));
    strncpy(dir, name, strlen(name) - strlen(filename));  //获取文件路径
    filename++;                                           //获取文件名
    int tp;
    if ((tp = searchDir(dir)) == -1) {
        cout << "no such directory" << endl;
        return;
    }
    for (int i = 0; i < 50; i++) {
        if (strcmp(disk[tp].Dire_Block->directory[i].name, filename) == 0) {
            deleteFileHelp(tp, i);
        }
    }
}

bool deleteDir(char* dir) {
    if (strcmp(dir, disk[1].super_block->cwdir) == 0 ||
        strstr(disk[1].super_block->cwdir, dir) != NULL ||
        strcmp(dir, disk[1].super_block->usrdir) == 0) {
        cout << "Permission denied" << endl;
        return false;
    }
    int p;
    char td[1024];
    strcpy(td, dir);
    if ((p = searchDir(dir)) == -1) {
        cout << "Directory doesn't exist" << endl;
        return false;
    }
    bool flag = true;
    for (int i = 0; i < 50; i++)  // delete file
    {
        if (strlen(disk[p].Dire_Block->directory[i].name) > 0) {
            int inode = disk[p].Dire_Block->directory[i].inode_number;
            if (disk[inode].i_node->type == 1)
                flag = deleteFileHelp(p, i);
            else {
                char temp[1024];
                strcpy(temp, td);
                strcat(temp, "/");
                strcat(temp, disk[p].Dire_Block->directory[i].name);
                flag = deleteDir(temp);
            }
        }
    }
    if (flag) {
        delete disk[p].Dire_Block;
        disk[1].super_block->free_data_block++;
        disk[3].data_bit_map->data_bit_map[p - SYSTEM_USED] = false;

        char* sub1 = strrchr(td, '/');
        char sub2[1024];
        memset(sub2, 0, sizeof(sub2));
        strncpy(sub2, td, strlen(td) - strlen(sub1));
        sub1++;
        int tp = searchDir(sub2);
        int tp1;
        for (int i = 0; i < 50; i++) {
            if (strcmp(sub1, disk[tp].Dire_Block->directory[i].name) == 0) {
                tp1 = disk[tp].Dire_Block->directory[i].inode_number;
                strcpy(disk[tp].Dire_Block->directory[i].name, "");
                disk[tp].Dire_Block->directory[i].inode_number = 0;
                break;
            }
        }
        delete disk[tp1].i_node;
        disk[tp].i_node = new I_NODE;
        disk[2]
            .i_node_bit_map
            ->inode_bit_map[tp - disk[1].super_block->first_inode_block] =
            false;
    } else {
        cout << "can't no delete locked file" << endl;
        return false;
    }
    return flag;
}

void listFile(char* dir) {
    if (strstr(dir, disk[1].super_block->usrdir) == NULL) {
        cout << "Permission denied" << endl;
        return;
    }
    int p;
    if ((p = searchDir(dir)) == -1) {
        cout << "Directories doesn't exist" << endl;
        return;
    }
    cout << "#################################################" << endl;
    for (int i = 0; i < 50; i++) {
        if (strlen(disk[p].Dire_Block->directory[i].name) > 0) {
            int inode = disk[p].Dire_Block->directory[i].inode_number;
            cout << "Name : " << disk[p].Dire_Block->directory[i].name << endl;
            if (disk[inode].i_node->type == I_NODE_TYPE_DIR) {
                cout << "Data type : directory" << endl;
                cout << "Size : 1KB" << endl;
            } else {
                cout << "Data type : user file" << endl;
                cout << "Current size : " << disk[inode].i_node->current_size << " Bytes" << endl;
                cout << "Max size : " << disk[inode].i_node->max_size << "Bytes" << endl;
            }
            cout << "Create time : " << ctime(&disk[inode].i_node->create_time);
            cout << "Last access time : "
                 << ctime(&disk[inode].i_node->access_time);
            cout << "#################################################" << endl;
        }
    }
}

void copyFile(char* obj, char* dest) {
    int p;
    if ((p = searchFile(obj)) == -1) {
        cout << "Source file doesn't exist in this directory!" << endl;
        return;
    }
    int size = disk[p].i_node->max_size;
    char* buf = new char[disk[p].i_node->current_size];
    memset(buf, 0, sizeof(buf));
    int tp;
    for (int i = 0; i < 10; i++) {
        tp = (int)disk[p].i_node->direct_addr[i];
        if (tp == 0)
            break;
        strcat(buf, disk[tp].data->content);
    }
    tp = (int)disk[p].i_node->indirect_addr[0];
    int tp1;
    if (tp != 0) {
        for (int i = 0; i < 256; i++) {
            tp1 = (int)disk[tp].in_addr->addr[i];
            if (tp1 == 0)
                break;
            strcat(buf, disk[tp1].data->content);
        }
    }
    tp = (int)disk[p].i_node->indirect_addr[1];
    int tp2;
    if (tp != 0) {
        for (int i = 0; i < 256; i++) {
            tp1 = (int)disk[tp].in_addr->addr[i];
            if (tp1 == 0)
                break;
            for (int j = 0; j < 256; j++) {
                tp2 = (int)disk[tp1].in_addr->addr[i];
                if (tp2 == 0)
                    break;
                strcat(buf, disk[tp2].data->content);
            }
        }
    }
    createFile(dest, size, buf);
}

void sum() {
    cout << "system used space : 191kb" << endl;
    cout << "Occupied blocks : "
         << (disk[1].super_block->total_data_block -
             disk[1].super_block->free_data_block) << endl;
    cout << "Available blocks : " << disk[1].super_block->free_data_block << endl;
    cout << "Total blocks for user data : " << disk[1].super_block->total_data_block << endl;
}

void changeDir(char* dir) {
    if (strstr(dir, disk[1].super_block->usrdir) == NULL) {
        cout << "Permission denied" << endl;
        return;
    }
    char td[1024];
    strcpy(td, dir);
    if (searchDir(td) != -1)
        strcpy(disk[1].super_block->cwdir, dir);
    else
        cout << "Directory doesn't exist" << endl;
}

void showWelcomeMsg() {
    cout << "welcome to login our simple file system" << endl;
    cout << "@copyright: 201830570453 Yunhui Zhang, 201830570460 Ziyang Zhou" << endl << endl;
}

void receiveCreateFileInput() {
    char filename[1024];
    memset(filename, 0, sizeof(filename));
    int size = 0;
    cin >> filename >> size;
    fixDirPath(filename);
    if (size >= disk[1].super_block->free_data_block * 1024) {
        cout << "error:file size greater available space" << endl;
        return;
    }
    cin.sync();  //清空输入缓存；
    cout << "please input the content of the file:" << endl;
    int count = 0;
    char* buff = new char[size];
    memset(buff, 0, size);
    while (cin.get(buff[count])) {
        if (buff[count] == '\n')
            break;
        count++;
        if (count == size - 1) {
            cout << "can not over the largest file size" << endl;
            break;
        }
    }
    buff[count] = '\0';
    createFile(filename, size, buff);
    cin.sync();
}

void receiveCMD(bool& flag) {
    char cmd[1024];
    cout << disk[1].super_block->cwdir << ">";
    cin.sync();
    cin >> cmd;

    if (strcmp(cmd, "createFile") == 0) {
        receiveCreateFileInput();
    }
    
    if (strcmp(cmd, "deleteFile") == 0) {
        char filename[1024];
        memset(filename, 0, sizeof(filename));
        cin >> filename;
        fixDirPath(filename);
        deleteFile(filename);
    }

    if (strcmp(cmd, "createDir") == 0) {
        char dir[1024];
        memset(dir, 0, sizeof(dir));
        cin >> dir;
        fixDirPath(dir);
        createDirectory(dir);
    }

    if (strcmp(cmd, "deleteDir") == 0) {
        char dir[1024];
        memset(dir, 0, sizeof(dir));
        cin >> dir;
        fixDirPath(dir);
        deleteDir(dir);
    }

    if (strcmp(cmd, "changeDir") == 0) {
        char dir[1024];
        cin >> dir;
        changeDir(dir);
    }

    if (strcmp(cmd, "dir") == 0) {
        char dir[1024];
        memset(dir, 0, sizeof(dir));
        cin >> dir;
        listFile(dir);
    }

    if (strcmp(cmd, "cp") == 0) {
        char dest[1024];
        char sour[1024];
        cin >> sour >> dest;
        fixDirPath(sour);
        copyFile(sour, dest);
    }

    if (strcmp(cmd, "sum") == 0) {
        sum();
    }

    if (strcmp(cmd, "cat") == 0) {
        char filename[1024];
        memset(filename, 0, sizeof(filename));
        cin >> filename;
        fixDirPath(filename);
        cat(filename);
    }

    if (strcmp(cmd, "exit") == 0) {
        flag = false;
    }
}

int main() {
    systemBoot();
    showWelcomeMsg();
    showhelp();

    bool mainLoop = true;
    while (mainLoop) {
        receiveCMD(mainLoop);
    }
}