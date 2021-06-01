#include <iostream>
#include <ctime>
#include <string.h>
#include <math.h>
#include <iomanip>

#define MAX_BLOCK 16 * 1024 //block的总数目
#define MAX_FILE_NUM_IN_DIR 50
#define SYSTEM_USED 1642

struct SUPER_BLOCK {
	int free_inode;
	int free_data_block;
	int total_data_block;
	int first_data_block;
	int first_inode_block;
	int last_inode_block;
	int inode_bitmap_block;
	int data_bitmap_block;
	int sizeof_block;
	char cwdir[1024];
	char usrdir[1024];
};

enum I_NODE_TYPE {I_NODE_TYPE_DIR, I_NODE_TYPE_FILE};

struct I_NODE {
    I_NODE_TYPE type;  //	0 代表dir, 1代表data
    time_t create_time;
	time_t access_time;
	int current_size;
	int max_size;
	int direct_addr[10];
	int indirect_addr[2];
}; //100bytes

struct DATA_BIT_MAP {
	bool data_bit_map[MAX_BLOCK - SYSTEM_USED];
};

struct INODE_BIT_MAP {
	bool inode_bit_map[MAX_BLOCK - SYSTEM_USED];
};

struct INDIRECT_ADDR_BLOCK {
	long addr[256];
}; //1KB

struct DATA_BLOCK {
	char content[1024];
}; 

struct DIRECTORY {
	long inode_number;
	char name[16];
};

struct DIRECTORY_BLOCK {
	char name[16];
	DIRECTORY directory[MAX_FILE_NUM_IN_DIR];
};

struct DISK {
	SUPER_BLOCK *super_block;
	INODE_BIT_MAP *i_node_bit_map;
	DATA_BIT_MAP *data_bit_map;
	I_NODE *i_node;

	//three types of the data blocks
	INDIRECT_ADDR_BLOCK *in_addr;
	DATA_BLOCK *data;
	DIRECTORY_BLOCK *Dire_Block;
} * disk; //具体的block类型按需求赋值；