#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile inodeNumber" << endl;
    return 1;
  }

  // Parse command line arguments
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  int inodeNumber = stoi(argv[2]);
  
  //-------------------GET INODE
  inode_t curr_inode;
  int stat_succ = fileSystem->stat(inodeNumber, &curr_inode);
  if(stat_succ!=0) {
    cerr<<"Error reading file\n";
    delete fileSystem;
    delete disk;
    return 1;
  }
  if(curr_inode.type == UFS_DIRECTORY) {
    cerr<<"Error reading file\n";
    delete fileSystem;
    delete disk;
    return 1;
  }

  //-------------------BLOCK NUMS
  cout<<"File blocks"<<endl;
  int file_size = curr_inode.size;
  int num_blks = file_size/UFS_BLOCK_SIZE;
  //include extra bytes? all blocks that contain any data from this inode?
  int overflow_bytes = file_size%UFS_BLOCK_SIZE;
  if(overflow_bytes>0) {
    num_blks+=1;
  }
  for(int i=0; i<num_blks; i++) {
    cout<<curr_inode.direct[i]<<endl;
  }
  cout<<endl;

  //-------------------FILE DATA
  cout<<"File data"<<endl;
  
  unsigned char contents[file_size];
  fileSystem->read(inodeNumber, contents, file_size);
  //file_size
  // string cont = "";
  for(int byte=0; byte<file_size; byte++) {
    cout<<contents[byte];
    // cont+=contents[byte];
    // cout<<(char)contents[byte];
  }
  // cout<<"\n";
  
  // cerr<<"Error reading file";
  // return 1;

  delete fileSystem;
  delete disk;

  return 0;
}
