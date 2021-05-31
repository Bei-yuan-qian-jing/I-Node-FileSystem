#include <iostream>
#include <ctime>
#include <string.h>
#include <math.h>
#include <iomanip>

#define MAX_BLOCK 16 * 1024; //block的总数目
int systemUsed = 1642;

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

struct I_NODE {
	int type;			//0 is directory ,1 is data;
	time_t create_time;
	time_t access_time;
	int current_size;
	int max_size;
	int direct_addr[10];
	int indirect_addr[2];
}; //100bytes

struct DATA_BIT_MAP {
	bool data_bit_map[16 * 1024 - 1642];
};

struct INODE_BIT_MAP {
	bool inode_bit_map[16 * 1024 - 1642];
};

struct INDIRECT_ADDR_BLOCK {
	long addr[256];
}; //1KB

struct DATA_BLOCK {
	char content[1024];
}; //1KB
//3是databit_map 2是inode_bitmap

struct DIRECTORY {
	long inode_number;
	char name[16];
};

struct DIRECTORY_BLOCK {
	char name[16];
	DIRECTORY directory[50];
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