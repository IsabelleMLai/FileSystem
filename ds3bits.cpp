#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 2) {
    cerr << argv[0] << ": diskImageFile" << endl;
    return 1;
  }

  // Parse command line arguments
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);

  //------------------READ SUPER BLOCK 
  super_t super_blk;
  fileSystem->readSuperBlock(&super_blk);
  
  //------------------PRINT SUPER BLOCK 
  cout<<"Super"<<endl;
  cout<<"inode_region_addr "<<super_blk.inode_region_addr<<"\n";
  cout<<"inode_region_len "<<super_blk.inode_region_len<<"\n";
  cout<<"num_inodes "<<super_blk.num_inodes<<"\n";

  cout<<"data_region_addr "<<super_blk.data_region_addr<<"\n";
  cout<<"data_region_len "<<super_blk.data_region_len<<"\n";
  cout<<"num_data "<<super_blk.num_data<<"\n";    //in # of blocks
  cout<<endl;

  //------------------READ BITMAPS
  int num_inode_bytes = (super_blk.num_inodes)/8;
  int num_data_bytes = (super_blk.num_data)/8;

  unsigned char inode_bm[num_inode_bytes];
  unsigned char data_bm[num_data_bytes];
  
  fileSystem->readInodeBitmap(&super_blk, inode_bm);
  fileSystem->readDataBitmap(&super_blk, data_bm);

  //------------------ PRINT BITMAPS
  cout<<"Inode bitmap\n";
  for(int byte=0; byte<num_inode_bytes; byte++) {
    cout<<  (unsigned int)inode_bm[byte] << " ";
  }
  cout<<endl<<endl;
  cout<<"Data bitmap\n";
  for(int byte=0; byte<num_data_bytes; byte++) {
    cout<<  (unsigned int)data_bm[byte] << " ";
  }
  cout<<endl;
  
  delete fileSystem;
  delete disk;

  return 0;
}
