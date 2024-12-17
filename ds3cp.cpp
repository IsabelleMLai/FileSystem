#include <iostream>
#include <string>
#include <cstring>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>


#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << argv[0] << ": diskImageFile src_file dst_inode" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img dthread.cpp 3" << endl;
    return 1;
  }

  // Parse command line arguments

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  string srcFile = "C:/Users/User/Desktop/";
  srcFile+=string(argv[2]);
  // int dstInode = stoi(argv[3]);
  
  char src[srcFile.length()];
  for(int  i=0; i<(int)srcFile.length();  i++) {
    src[i]= srcFile[i];
  }
  int fd = open(src, O_RDONLY);
  cout<<fd<<" "<<endl;

  // int buff_size = 10;
  // char buff[buff_size];
  // int num_bytes_read = read(fd, buff, 10);
  // cout<<num_bytes_read<<endl;

  // // fileSystem->write(dstInode, buff, 10 );
  // cout<<(fileSystem->write(dstInode, buff, 10 ));
  
  delete fileSystem;
  delete disk;
  return 0;
}
