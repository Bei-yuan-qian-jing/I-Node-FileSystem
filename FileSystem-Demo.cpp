#include <iostream>
#include<ctime>
#include<string.h>
#include<math.h>
#include<iomanip>
using namespace std;
#define MAX_BLOCK 16*1024;//block的总数目
int systemUsed =1642;
struct USER_INF{
char user_id[22];
char pass_word[20];
};
struct BOOT_BLOCK{
	USER_INF user_inf [3];
	int user_sum;
	int current_user;
};
struct SUPER_BLOCK{
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
struct I_NODE{
    int uid;
	int lock;//0 can be delete,1can;'t delete
	int read_only_flag;//0 can be for read and wirte ,1 read only.
	int type;//0 is directory ,1 is data;
	time_t create_time;
	time_t modification_time;
	time_t access_time;
	int current_size;
	int max_size;
	int direct_addr[10];
	int indirect_addr[2];
	int shareDir;
	int shareOffset;
};//100bytes
struct DATA_BIT_MAP{
bool data_bit_map[16*1024-1642];
};
struct INODE_BIT_MAP{
	bool inode_bit_map[16*1024-1642];
};
struct INDIRECT_ADDR_BLOCK{
long addr[256];
};//1KB
struct DATA_BLOCK{
 char content[1024];
};//1KB
//3是databit_map 2是inode_bitmap
struct DIRECTORY{
long inode_number;
char name[16];
};
struct DIRECTORY_BLOCK
{ 
	char name[16];
	DIRECTORY directory[50];
};
struct DISK{
BOOT_BLOCK *boot_block;
SUPER_BLOCK *super_block;
INODE_BIT_MAP *i_node_bit_map;
DATA_BIT_MAP *data_bit_map;
I_NODE *i_node;

//three types of the data blocks
INDIRECT_ADDR_BLOCK *in_addr;
DATA_BLOCK *data;
DIRECTORY_BLOCK *Dire_Block;
} *disk;//具体的block类型按需求赋值；

/*############################################*/
//help function!
int findFile_INode(long p, char name[])//return file's inode
{
	int pointer = -1;
	time_t ti;
	for(int i=0;i<50;i++)
	{
		if(strcmp(disk[p].Dire_Block->directory[i].name,name) == 0)
		{
			pointer = disk[p].Dire_Block->directory[i].inode_number;
			time(&ti);
			disk[pointer].i_node->access_time = ti;
			return pointer;
		}
	}
	return pointer;
}
int findFreeINode()
{
	for(int i= 0;i<disk[1].super_block->total_data_block;i++)
		if(disk[2].i_node_bit_map->inode_bit_map[i]==false)
			return i+disk[1].super_block->first_inode_block;
}
int findFreeDataBlock()
{
	for(int i= 0;i<disk[1].super_block->total_data_block;i++)
		if(disk[3].data_bit_map->data_bit_map[i]==false)
			return i+systemUsed;
}
DIRECTORY_BLOCK*  newDirectory()
{
DIRECTORY_BLOCK* temp = new DIRECTORY_BLOCK ;
for(int i=0;i<50;i++)
		memset(temp->directory[i].name,0,16);
return temp;
}
 INDIRECT_ADDR_BLOCK* newINDIR_Addr()
 {
  INDIRECT_ADDR_BLOCK*temp = new INDIRECT_ADDR_BLOCK;
  for(int i=0;i<256;i++)
	  temp->addr[i]=0;
  return temp;
 }
void assign_INode(int I)
{
	time_t t;
	time(&t);
	disk[I].i_node->create_time =disk[I].i_node->access_time= disk[I].i_node->modification_time=t;
	disk[I].i_node->uid =disk[0].boot_block->current_user;
	disk[I].i_node->current_size=0;
	disk[I].i_node->lock = 0;
	disk[I].i_node->shareDir = 0;
	disk[I].i_node->read_only_flag = 0;
	for(int i=0;i<10;i++)
		disk[I].i_node->direct_addr[i]=0;
	for(int i=0;i<2;i++)
		disk[I].i_node->indirect_addr[i]=0;
}
void make_Dir(long &p,char*token)
{

	for(int i=0;i<50;i++)
				  {
					
					if(strlen(disk[p].Dire_Block->directory[i].name)==0)
					  {
						strcpy(disk[p].Dire_Block->directory[i].name,token);
						int Inode =findFreeINode();
						disk[p].Dire_Block->directory[i].inode_number = Inode;
						assign_INode(Inode);//request inode for new directory
						disk[Inode].i_node->type = 0;
						disk[2].i_node_bit_map->inode_bit_map[Inode-disk[1].super_block->first_inode_block]=true;

						int data_p= findFreeDataBlock();//request data block for new directory
						disk[1].super_block->free_data_block--;
						disk[Inode].i_node->direct_addr[disk[Inode].i_node->current_size++]=data_p;
						disk[data_p].Dire_Block= newDirectory();
						disk[3].data_bit_map->data_bit_map[data_p-systemUsed]=true;
						strcpy(disk[data_p].Dire_Block->name,token);

						p=data_p;
						break;
					  }
				  }
}
void make_inDir( INDIRECT_ADDR_BLOCK*p,char*filecontent,int&totalBlocks,int&used_Blocks)
{
	int count =totalBlocks<256?totalBlocks:256;
	int temp;
	for(int i=0;i<count;i++)
	{
		    temp=p->addr[i] = findFreeDataBlock();
			disk[1].super_block->free_data_block--;
			disk[3].data_bit_map->data_bit_map[temp-systemUsed]=true;
			disk[temp].data=new DATA_BLOCK;
			memset(disk[temp].data->content,0,1024);
			totalBlocks--;
			if(used_Blocks>0)//copy the file content into the new space;
		   {
			   used_Blocks--;
			    strncpy(disk[temp].data->content,filecontent,1024);
				filecontent+=1024;
			}
	}

}
void make_File(int &p,char*token,double size,char*filecontent)
{
	int Inode;
	for(int i=0;i<50;i++)//find the proper palce in directory for the filename and allowcate inode 
				  {
					if(strlen(disk[p].Dire_Block->directory[i].name)==0)
					  {
						strcpy(disk[p].Dire_Block->directory[i].name,token);
					    Inode =findFreeINode();
						if (Inode == -1)//not free Inode
							return;
						disk[p].Dire_Block->directory[i].inode_number = Inode;
						assign_INode(Inode);//allowcate inode for new directory
						disk[Inode].i_node->type = 1;
						disk[2].i_node_bit_map->inode_bit_map[Inode-disk[1].super_block->first_inode_block]=true;
						p=Inode;
						break;
						
					  }
	              }   
			           int usedSpace=disk[p].i_node->current_size =strlen(filecontent);
					   disk[p].i_node->max_size = size;
					    int used_Blocks=(int)ceil((double)usedSpace/1024);
						int totalBlocks=(int)ceil(size/1024);
						int used_Blocks2 = used_Blocks;
						
						int count =totalBlocks<10?totalBlocks:10;
						int temp;
						for(int i=0;i<count;i++)
						{
							temp=disk[Inode].i_node->direct_addr[i] = findFreeDataBlock();
						    disk[3].data_bit_map->data_bit_map[temp-systemUsed]=true;
							disk[temp].data=new DATA_BLOCK;
							memset(disk[temp].data->content,0,1024);
							disk[1].super_block->free_data_block--;
							totalBlocks--;
							if(used_Blocks>0)
							{
							used_Blocks--;
							strncpy(disk[temp].data->content,filecontent,1024);
							filecontent+=1024;
							}
						}
					
						if(totalBlocks>0)//第一级indirect directory;
						{
						int new_p= findFreeDataBlock();//request data block for new data;
						disk[new_p].in_addr = newINDIR_Addr();
						disk[Inode].i_node->indirect_addr[0]=new_p;
						disk[3].data_bit_map->data_bit_map[new_p-systemUsed]=true;
						make_inDir(disk[new_p].in_addr,filecontent,totalBlocks,used_Blocks);
					}
				if(totalBlocks>0)//第二级indirect address
				{
				    	int new_p1= findFreeDataBlock();//request data block for new data;
						disk[new_p1].in_addr = newINDIR_Addr();
						disk[Inode].i_node->indirect_addr[1]=new_p1;
						disk[3].data_bit_map->data_bit_map[new_p1-systemUsed]=true;
						int count=0;
						while(totalBlocks>0&&count<256)
						{
						 int new_p2= findFreeDataBlock();//request data block for new data;
						 disk[new_p2].in_addr = newINDIR_Addr();
					     disk[new_p1].in_addr->addr[count++]=new_p2;
						 disk[3].data_bit_map->data_bit_map[new_p2-systemUsed]=true;
						 make_inDir(disk[new_p2].in_addr,filecontent,totalBlocks,used_Blocks);
					    }
				}
				filecontent-=used_Blocks2*1024;
			   // delete []filecontent;
}
bool deleteFileHelp(int &p,int i)
{   
	int inode;
	inode=disk[p].Dire_Block->directory[i].inode_number ;//get the I_node of the file
	if (disk[inode].i_node->lock == 1)//check wheter the file is locked
	{
		cout << disk[p].Dire_Block->directory[i].name<<" is locked ,permission denied" << endl;
		return false;
	}
	 strcpy(disk[p].Dire_Block->directory[i].name,"");
	disk[p].Dire_Block->directory[i].inode_number =0;//在目录上清除
	
	if (disk[inode].i_node->shareDir != 0)//delete soft link
	{
		int p = disk[inode].i_node->shareDir;
		int o = disk[inode].i_node->shareOffset;
		strcpy(disk[p].Dire_Block->directory[o].name, "");
		disk[p].Dire_Block->directory[o].inode_number = 0;
		return true;
	}

		int tp;//delete data
		for(int i=0;i<10;i++)
		{
			tp=(int)disk[inode].i_node->direct_addr[i];
			if(tp==0)
				break;
			delete disk[tp].data;
			disk[3].data_bit_map->data_bit_map[tp-systemUsed]=false;
			disk[1].super_block->free_data_block++;
	
		}

		tp=(int)disk[inode].i_node->indirect_addr[0];
		int tp1;
		if(tp!=0)
		{
		for(int i=0;i<256;i++)
		{
			tp1=(int)disk[tp].in_addr->addr;
			if(tp1==0)
				break;
			delete disk[tp1].data;
			disk[3].data_bit_map->data_bit_map[tp1-systemUsed]=false;
			disk[1].super_block->free_data_block++;
		}
		}
	
		tp=(int)disk[inode].i_node->indirect_addr[1];
		if(tp!=0)
		{
		int tp2;
		for(int i=0;i<256;i++)
		{
			tp1 = disk[tp].in_addr->addr[i];
			if(tp1==0)
				break;
			for(int j=0;j<256;j++)
			{
				tp2=disk[tp1].in_addr->addr[j];
				if(tp2==0)
					break;
				delete disk[tp2].data;
				disk[3].data_bit_map->data_bit_map[tp2-systemUsed]=false;
				disk[1].super_block->free_data_block++;
			}
		 }
		}
		disk[2].i_node_bit_map->inode_bit_map[inode-disk[1].super_block->first_inode_block]=false;
	delete disk[inode].i_node;
	disk[inode].i_node=new I_NODE;
	return true;
}
int searchFile(char*filename)//返回文件 的Inode
{
	int pointer = -1;
	char *token = strtok(filename, "/");
	long p = disk[1].super_block->first_data_block;
	token = strtok(NULL, "/");
	int tp;
	while (token != NULL)
	{
		if ((tp = findFile_INode(p, token)) == -1)
			return pointer;
		p = disk[tp].i_node->direct_addr[0];
		token = strtok(NULL, "/");
	}
	pointer = tp;
	return pointer;
}
int searchDir(char*dir)//返回目录文件的地址
{
	long pointer = -1;
   char *sub = strtok(dir, "/");
	long p = disk[1].super_block->first_data_block;
	sub = strtok(NULL, "/");
	int tp;
	while (sub != NULL)
	{
		if ((tp = findFile_INode(p, sub)) == -1)
			return pointer;
		p = disk[tp].i_node->direct_addr[0];//获取下一级目录的地址
		
		sub = strtok(NULL, "/");
	}
	pointer = p;
	return pointer;
}
void dirFix(char*dir)
{
	if (strchr(dir, '/') == NULL)
	{
		char temp[1024];
		strcpy(temp, dir);
		strcpy(dir, disk[1].super_block->cwdir);
		strcat(dir, "/");
		strcat(dir, temp);
	}
}
/*############################################*/

void showhelp();
void createRoot()
{
	int p = disk[1].super_block->first_data_block;
	disk[p].Dire_Block =newDirectory();
	disk[1].super_block->free_data_block --;
	disk[3].data_bit_map->data_bit_map[0]=true;
	strcpy(disk[p].Dire_Block->name,"root");
	strcpy(disk[1].super_block->usrdir, disk[p].Dire_Block->name);
}
void format()
{
	disk= new DISK[16*1024];
	disk[0].boot_block = new BOOT_BLOCK;
	disk[0].boot_block->user_sum=1;
	disk[0].boot_block->current_user = 0;
	strcpy(disk[0].boot_block->user_inf[0].user_id,"admin");
	strcpy(disk[0].boot_block->user_inf[0].pass_word,"admin");//在boot_block中初始化管理员信息；
	strcpy(disk[0].boot_block->user_inf[1].user_id, "user1");
	strcpy(disk[0].boot_block->user_inf[1].pass_word, "user1");//在boot_block中初始化用户信息；
	strcpy(disk[0].boot_block->user_inf[2].user_id, "user2");
	strcpy(disk[0].boot_block->user_inf[2].pass_word, "user2");//在boot_block中初始化用户信息；
    
	disk[1].super_block = new SUPER_BLOCK;
	disk[1].super_block->free_inode=1638;
	disk[1].super_block->free_data_block =16194;
	disk[1].super_block->total_data_block =16194;
	disk[1].super_block->sizeof_block= 1024;
   disk[1].super_block->inode_bitmap_block = 2;
   disk[1].super_block->data_bitmap_block = 3;
   disk[1].super_block->first_inode_block = 4;
   disk[1].super_block->last_inode_block=1641;
   disk[1].super_block->first_data_block = 1642;
    disk[2].i_node_bit_map= new INODE_BIT_MAP;
	memset(disk[2].i_node_bit_map->inode_bit_map,0,sizeof(disk[2].i_node_bit_map->inode_bit_map));
	disk[3].data_bit_map = new DATA_BIT_MAP;
	memset(disk[3].data_bit_map->data_bit_map,0,sizeof(disk[3].data_bit_map->data_bit_map));
	for(int i=4;i<systemUsed;i++)
		{
			disk[i].i_node = new I_NODE;
	    }
	disk[1].i_node_bit_map = new INODE_BIT_MAP;
    createRoot();//程序启动创建根文件夹。
}
int createDirectory(char dir[])
{
	
	int pointer=-1;
	if (strstr(dir, disk[1].super_block->usrdir)!= NULL)//如果是在当前用户的目录下创建新路径，则请求被允许
	{
		char *token = strtok(dir, "/");
		long p = disk[1].super_block->first_data_block;
		token=strtok(NULL,"/");
		int tp;
		int ltp=0;
	      while(token!=NULL)
	       {
			  if ((tp = findFile_INode(p, token)) == -1)//看看是否找到与token对应的inode
			  {
				  make_Dir(p, token); //如果没有找到则在该级路径想创建新的文件夹
				  time_t ti;
				  time(&ti);
				  if (ltp!=0)
				  disk[ltp].i_node->modification_time = ti;
			  }
			  else//如果已经找到
			  {
				  p = disk[tp].i_node->direct_addr[0];//通过inode访问下一级路径
				  ltp = tp;
			  }
			       token=strtok(NULL,"/");
			  }  ;
		pointer=p;
       }
      else cout<<"Permission denied!"<<endl;
	return pointer;
}
void createFile(char*name,int size,char*filecontent)
{

	if (strstr(name, disk[1].super_block->usrdir) == NULL)
	{ 
		cout << "Permission denied" << endl;
		return;
	}
char *filename =strrchr(name,'/');
char dir[1024];
memset(dir,0,sizeof(dir));
strncpy(dir,name,strlen(name)-strlen(filename));//获取文件路径
filename++;//获取文件名

int pointer;
if((pointer=createDirectory(dir))==-1)
{
cout<<"Directory creating denied"<<endl;
}
else{   
                  for(int i=0;i<50;i++)
					  if(strcmp(disk[pointer].Dire_Block->directory[i].name,filename)==0)
					  {
					  cout<<"Same file name in the directory，would you like to replace it<y/n>"<<endl;
					  char n;
					  cin>>n;
					  if(n=='n')
					    return;
					  else if( !deleteFileHelp(pointer,i))
						        return;
					  }
					make_File(pointer,filename,size,filecontent);
		}
}
void cat(char*filename)
{
int p;
if (strstr(filename, disk[1].super_block->usrdir) == NULL)
{
	cout << "Permission denied" << endl;
	return;
}
 if((p=searchFile(filename))==-1)
	{
		cout<<"file doesn't exist in this directory!"<<endl;
		return;
    }
 int tp;
for(int i=0;i<10;i++)
{
	tp=(int)disk[p].i_node->direct_addr[i];
	if(tp==0)
		break;
	cout<<disk[tp].data->content;
}
tp =(int)disk[p].i_node->indirect_addr[0];
int tp1;
if (tp != 0)
{

	for (int i = 0; i < 256; i++)
	{
		tp1 = (int)disk[tp].in_addr->addr[i];
		if (tp1 == 0)
			break;
		cout << disk[tp1].data->content;
	}
}
tp =(int)disk[p].i_node->indirect_addr[1];
int tp2;
if (tp != 0)
{
	for (int i = 0; i < 256; i++)
	{
		tp1 = (int)disk[tp].in_addr->addr[i];
		if (tp1 == 0)
			break;
		for (int j = 0; j < 256; j++)
		{
			tp2 = (int)disk[tp1].in_addr->addr[i];
			if (tp2 == 0)
				break;
			cout << disk[tp2].data->content;
		}
	}
}
cout << endl;
}
void deleteFile(char*name)
{
	if (strstr(name, disk[1].super_block->usrdir) == NULL)
	{
		cout << "Permission denied" << endl;
		return;
	}
	char *filename = strrchr(name, '/');
	char dir[16];
	memset(dir, 0, sizeof(dir));
	strncpy(dir, name, strlen(name) - strlen(filename));//获取文件路径
	filename++;//获取文件名
    int tp;
	if ((tp=searchDir(dir))==-1)
	{
		cout << "not such a directory" << endl;
		return;
	}
	for (int i = 0; i<50; i++)
	if (strcmp(disk[tp].Dire_Block->directory[i].name, filename) == 0)
	{
		deleteFileHelp(tp, i);
	}
}
bool deleteDir(char*dir)
{
	if (strcmp(dir, disk[1].super_block->cwdir) == 0 || strstr(disk[1].super_block->cwdir, dir) != NULL || strcmp(dir, disk[1].super_block->usrdir)==0)
	{
		cout << "Permission denied" << endl;
		return false;
	}
	int p;
	char td[1024];
    strcpy(td,dir);
	if ((p = searchDir(dir)) == -1)
	{  
		cout << "Directory doesn't exist" << endl;
		return false;
	}
	bool flag = true;
	for (int i = 0; i < 50; i++)//delete file
	{
		if (strlen(disk[p].Dire_Block->directory[i].name)>0)
		{
			int inode = disk[p].Dire_Block->directory[i].inode_number;
			if (disk[inode].i_node->type == 1)
				flag = deleteFileHelp(p, i);
			else{
				char temp[1024];
				strcpy(temp, td);
				strcat(temp, "/");
				strcat(temp, disk[p].Dire_Block->directory[i].name);
				flag = deleteDir(temp);
			
			}
		}
	}
	if (flag)
	{
		delete disk[p].Dire_Block;
		disk[1].super_block->free_data_block++;
		disk[3].data_bit_map->data_bit_map[p - systemUsed] = false;

		char *sub1 = strrchr(td, '/');
		char sub2[1024];
		memset(sub2, 0, sizeof(sub2));
		strncpy(sub2, td, strlen(td) - strlen(sub1));
		sub1++;
		int tp = searchDir(sub2);
		int tp1;
		for (int i = 0; i < 50; i++)
		{
			if (strcmp(sub1, disk[tp].Dire_Block->directory[i].name) == 0)
			{
				tp1 = disk[tp].Dire_Block->directory[i].inode_number;
				strcpy(disk[tp].Dire_Block->directory[i].name, "");
				disk[tp].Dire_Block->directory[i].inode_number = 0;
				break;
			}
		}
		delete disk[tp1].i_node;
		disk[tp].i_node = new I_NODE;
		disk[2].i_node_bit_map->inode_bit_map[tp - disk[1].super_block->first_inode_block] = false;
	}
	else {
		    cout << "can't no delete locked file" << endl;
			return false;
	        }
	
	return flag;
}
void listFile(char*dir)
{ 
	if (strstr(dir, disk[1].super_block->usrdir) == NULL)
	{
		cout << "Permission denied" << endl;
		return;
	}
	int p;
	if ((p = searchDir(dir)) == -1)
	{
		cout << "Directories doesn't exist" << endl;
		return;
	}
	cout << "#################################################" << endl;
	for (int i = 0; i < 50; i++)
	{
		if (strlen(disk[p].Dire_Block->directory[i].name)>0)
		{
			int inode = disk[p].Dire_Block->directory[i].inode_number;
			cout <<"Name : "<< disk[p].Dire_Block->directory[i].name << endl;
			cout << "Owner : " << disk[0].boot_block->user_inf[disk[inode].i_node->uid].user_id<<endl;
			if (disk[inode].i_node->type == 0)
			{
				cout << "Data type : directory" << endl;
				cout << "Size : 1KB" << endl;
			}
			else
			{
				cout << "Data type : user file" << endl;
				cout << "Current size : " << disk[inode].i_node->current_size << " Bytes" << endl;
				cout << "Max size : " << disk[inode].i_node->max_size << "Bytes" << endl;
				cout << "Lock state : ";
				if (disk[inode].i_node->lock == 1)
					cout << "Locked" << endl;
				else cout << "Unlocked" << endl;
			}
			cout << "Create time : " << ctime(&disk[inode].i_node->create_time);
			cout << "Last access time : " << ctime(&disk[inode].i_node->access_time) ;
			cout << "Last modification time : " << ctime(&disk[inode].i_node->modification_time) ;
			cout << "#################################################" << endl;
		}
	}
}
void cpyFile(char*obj, char*dest)
{
	int p;
	if ((p = searchFile(obj)) == -1)
	{
		cout << "Source file doesn't exist in this directory!" << endl;
		return;
	}
	int size = disk[p].i_node->max_size;
	char*buf = new char[disk[p].i_node->current_size];
	memset(buf, 0, sizeof(buf));
	int tp;
	for (int i = 0; i<10; i++)
	{
		tp = (int)disk[p].i_node->direct_addr[i];
		if (tp == 0)
			break;
		strcat(buf,disk[tp].data->content);
	}
	tp = (int)disk[p].i_node->indirect_addr[0];
	int tp1;
	if (tp != 0)
	{
	  for (int i = 0; i < 256; i++)
		{
			tp1 = (int)disk[tp].in_addr->addr[i];
			if (tp1 == 0)
				break;
			strcat(buf, disk[tp1].data->content);
		}
	}
	tp = (int)disk[p].i_node->indirect_addr[1];
	int tp2;
	if (tp != 0)
	{

		for (int i = 0; i < 256; i++)
		{
			tp1 = (int)disk[tp].in_addr->addr[i];
			if (tp1 == 0)
				break;
			for (int j = 0; j < 256; j++)
			{
				tp2 = (int)disk[tp1].in_addr->addr[i];
				if (tp2 == 0)
					break;
				strcat(buf, disk[tp2].data->content);
			}
		}
	}
	createFile(dest, size, buf);

}
void sum()
{
	cout << "system used space : 191kb" << endl;
	cout << "Occupied blocks : " << (disk[1].super_block->total_data_block - disk[1].super_block->free_data_block) << endl;
	cout << "Available blocks : " << disk[1].super_block->free_data_block << endl;
	cout << "Total blocks for user data : " << disk[1].super_block->total_data_block<<endl;
}
void changeDir(char*dir)
{
	if (strstr(dir, disk[1].super_block->usrdir) == NULL)
	{
		cout << "Permission denied" << endl;
		return;
	}
	char td[1024];
	strcpy(td, dir);
	if (searchDir(td)!=-1)
	strcpy(disk[1].super_block->cwdir, dir);
	else cout << "Directory doesn't exist" << endl;
}
void login(bool&flag)
{
	cout << "Login..................." << endl << "ID:admin,Password:admin;  " << endl << "ID:user1, Password : user1; " << endl << "ID:user1, Password : user2." << endl;
	cout << "User name : ";
	char user[1024];
	char password[1024];
	cin >> user;
	cout << "Password : ";
	cin >> password;
	for (int i = 0; i < 3; i++)
	{
		if (strcmp(disk[0].boot_block->user_inf[i].user_id, user) == 0 && strcmp(disk[0].boot_block->user_inf[i].pass_word, password) == 0)
		{
			disk[0].boot_block->current_user = i;
			system("cls");
			cout << "Login successful!" << endl;
			if (i>0)
			{
				char temp[1024] = { "/root/" };
				strcat(temp, user);
		        strcpy(disk[1].super_block->usrdir, temp);
				strcpy(disk[1].super_block->cwdir, disk[1].super_block->usrdir);
				createDirectory(temp);
				strcpy(temp, disk[1].super_block->cwdir);
				strcat(temp, "/share");
				createDirectory(temp);
			}
			else{
				    strcpy(disk[1].super_block->cwdir, "/root");
				     strcpy(disk[1].super_block->usrdir, "/root");
			        }
			showhelp();
			flag = false;
			return;
		}
	}
	cout << "Login denied" << endl;
}
bool changeState(char*mode,char*dir)
{
	int inode = searchFile(dir);
	if (strcmp(mode, "ul") == 0)
	{
		disk[inode].i_node->lock = 0;
		time_t t;
		time(&t);
		disk[inode].i_node->modification_time = t;
	}else
	if (strcmp(mode, "l") == 0)
	{
		disk[inode].i_node->lock = 1;
		time_t t;
		time(&t);
		disk[inode].i_node->modification_time = t;
	}
	else return false;
	return true;
}
bool readOnly(char*mode, char*dir)
{
	int inode = searchFile(dir);
	if (inode == -1)
	{
		cout << "file doesn't exist" << endl;
		return false;
	}
	if (strcmp(mode, "rw") == 0)
	{
		disk[inode].i_node->read_only_flag = 0;
		time_t t;
		time(&t);
		disk[inode].i_node->modification_time = t;
	}
	else
	if (strcmp(mode, "r") == 0)
	{
		disk[inode].i_node->read_only_flag= 1;
		time_t t;
		time(&t);
		disk[inode].i_node->modification_time = t;
	}
	else return false;
	return true;
}
void editFile(char*dir,char* buf)
{
	if (strstr(dir, disk[1].super_block->usrdir) == NULL)
	{
		cout << "Permission denied" << endl;
		return;
	}
	int inode= searchFile(dir);
	if (inode == -1)
	{
		cout << "file doesn't exist" << endl;
		return;
	}
	if (disk[inode].i_node->read_only_flag == 1)
	{
		cout << "permission denied" << endl;
		return;
	}
	if (strlen(buf) > disk[inode].i_node->max_size)
	{
		cout << "Not enough space" << endl;
		return;
	}
    time_t t;
	time(&t);
	disk[inode].i_node->modification_time = t;
    double  size = strlen(buf);
	disk[inode].i_node->current_size = size;
	int totalBlocks = ceil(size / 1024);
	int count = 10 < totalBlocks ? 10 : totalBlocks;
	for (int i = 0; i < count; i++)
	{
		int point = disk[inode].i_node->direct_addr[i];
	
		strncpy(disk[point].data->content, buf, 1024);
		buf += 1024;
		totalBlocks--;
	}
	if (totalBlocks == 0)
		return;
    count = 256< totalBlocks ? 256 : totalBlocks;
	int p1 = disk[inode].i_node->indirect_addr[0];
	int p2;
	for (int i = 0; i < count; i++)
	{
		p2 = disk[p1].in_addr->addr[i];
		
		strncpy(disk[p2].data->content, buf, 1024);
		buf += 1024;
		totalBlocks--;
	}
	if (totalBlocks == 0)
		return;
	int p3;
	p1 = disk[inode].i_node->indirect_addr[1];
	for (int i = 0; i < 256; i++)
	{
		p2 = disk[p1].in_addr->addr[i];
		for (int j = 0; j < 256; j++)
		{
			p3 = disk[p2].in_addr->addr[j];
			
			strncpy(disk[p3].data->content, buf, 1024);
			buf += 1024;
			totalBlocks--;
			if (totalBlocks == 0)
				return;
		}
	}
}
void share(char*dest, char*orig)
{
	long p1;
	if ((p1 = searchDir(dest)) == -1)
	{   
		cout << "Destination directory doesn't exist" << endl;
		return;
	}
	int inode;
	char temp[1024];
	strcpy(temp, orig);
	if ((inode = searchFile(temp)) == -1)
	{
		cout << "Source file doesn't exist" << endl;
		return ;
	};
	char *filename = strrchr(orig,'/');
	filename++;
	for (int i = 0; i < 50; i++)
	{
		if (strlen(disk[p1].Dire_Block->directory[i].name) == 0)
		{
			strcpy(disk[p1].Dire_Block->directory[i].name, filename);
			disk[p1].Dire_Block->directory[i].inode_number = inode;
			break;
		}
	}
	disk[inode].i_node->shareDir = p1;
}
void cmdF(bool& flag)
{
char cmd[1024];
cout << disk[0].boot_block->user_inf[disk[0].boot_block->current_user].user_id << "@ " << disk[1].super_block->cwdir << ">";
cin.sync();
cin>>cmd;
if(strcmp(cmd,"createF")==0)
{
char filename[1024];
memset(filename,0,sizeof(filename));
int size=0;
cin>>filename>>size;
dirFix(filename);
if(size>=disk[1].super_block->free_data_block*1024)
{
	cout<<"error:file size greater available space"<<endl;
	return;
}
cin.sync();//清空输入缓存；
cout<<"please input the content"<<endl;
int count = 0;
char* buff = new char[size];
memset(buff,0,size);
while(cin.get(buff[count] ))
{   
    if(buff[count]=='\n')
			break;
      count++;
	  if(count==size-1)
	  {
	  cout<<"can not over the largest file size"<<endl;
	  break;
	  }
}
buff[count] = '\0';
createFile(filename,size,buff);
cin.sync();
}

if(strcmp(cmd,"cat")==0)
{
char filename[1024];
memset(filename,0,sizeof(filename));
cin>>filename;
dirFix(filename);
cat(filename);
}

if (strcmp(cmd, "editF") == 0)
{
	char filename[1024];
	memset(filename, 0, sizeof(filename));
    cin >> filename;
	dirFix(filename);
	cin.sync();//清空输入缓存；
	cout << "please input the new content" << endl;
	int count = 0;
	char* buff = new char[1024];
	memset(buff, 0, 1024);
	while (cin.get(buff[count]))
	{
		if (buff[count] == '\n')
			break;
		count++;
		if (count == 1024 - 1)
		{
			cout << "can not over the largest file size" << endl;
			break;
		}
	}
	editFile(filename, buff);
	cin.sync();
}

if (strcmp(cmd, "deleteF")==0)
{
	char filename[1024];
	memset(filename, 0, sizeof(filename));
	cin >> filename;
	dirFix(filename);
	deleteFile(filename);
}

if (strcmp(cmd, "deleteD") == 0)
{
	char  dir[1024];
	memset(dir, 0, sizeof(dir));
	cin >> dir;
	dirFix(dir);
	deleteDir(dir);
}

if (strcmp(cmd, "createD") == 0)
{  
	char  dir[1024];
	memset(dir, 0, sizeof(dir));
	cin >> dir;
	dirFix(dir);
	createDirectory(dir);
}
if (strcmp(cmd, "listF") == 0)
{
	char  dir[1024];
	memset(dir, 0, sizeof(dir));
	cin >> dir;
	listFile(dir);
}
if (strcmp(cmd, "cpyF") == 0)
{
	char dest[1024];
	char sour[1024];
   cin >> sour >> dest;
	dirFix(sour);
	cpyFile(sour,dest);
}
if (strcmp(cmd, "share") == 0)
{ 
	char dest[1024];
	char orig[1024];
	cin>>orig>>dest;
	dirFix(orig);
	share(dest, orig);
}

if (strcmp(cmd, "sum")==0)
{
	sum(); 
}
if (strcmp(cmd, "cd")==0)
{
	char dir[1024];
	cin >> dir;
	changeDir(dir);
}
if (strcmp(cmd, "changeS") == 0)
{
	char dir[1024];
	char lock[10];
	cin >> dir  >> lock;
	dirFix(dir);
	if (!changeState(lock, dir))
		cout << "illegal command" << endl;
}
if (strcmp(cmd, "readOnly") == 0)
{
	char dir[1024];
	char lock[10];
	cin >> dir >> lock;
	dirFix(dir);
	if(!readOnly(lock, dir))
		cout<<"illegal command"<<endl;
}
if (strcmp(cmd, "logout")==0)
{
	bool flag1= true;
	while (flag1)
	login(flag1);
}
if (strcmp(cmd, "exit") == 0)
{
	flag = false;
}
}

int main()
{

format();

char *content=new char[1024];
strcpy(content,"this is file1");
createFile("/root/dir1/file1",1024, content);
char *content1 = new char[1024];
strcpy(content1, "this is file2");
createFile("/root/dir1/dir2/file2", 1024, content1);

bool flag = true;
while (flag)
login(flag);
flag = true;
while(flag)
 cmdF(flag);
return 0;

}
void showhelp()
{
	cout << "\thelp\t\tshow help infor" << endl;
	cout << "\tshare\t\tshare file to other user" << endl;
	cout << "\tcreateF\t\tcreate a new file" << endl;
	cout << "\tcat\t\topen a file" << endl;
	cout << "\tdeleteF\t\tdelete a file" << endl;
	cout << "\teditF\t\tedit file" << endl;
	cout << "\treadOnly\tset the file as read only flag ,r or rw " << endl;
	cout << "\tcreateD\t\tcreate a new directory" << endl;
	cout << "\tdeleteD\t\tdelete a directory" << endl;
	cout << "\tlistF\t\tlist all the file and sub directory information" << endl;
	cout << "\tcpyF\t\tcopy a file to other directory" << endl;
	cout << "\tsum\t\tshow the usage of space" << endl;
	cout << "\tcd\t\tchange the current working directory" << endl;
	cout << "\tchangeS\t\tchange the lock state l for lock ,ul for unlock" << endl;
	cout << "\tlogout\t\tlogout" << endl;
	cout << "\texit\t\texit" << endl;
};