#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

#include "StringUtils.h"
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

#define INT_SIZE (4)
#define DIR_ENT_SIZE_BYTES  (DIR_ENT_NAME_SIZE + INT_SIZE)

/*
DOEESNN'T  HANDLE ROOT DIR LS
- should make . = 0, and ..=0
*/

/*
- finds index of slash, includes start_ind
- if can't find slash or start_ind out of bounds, returns -1
*/
int FindSlashIndex(string dir, int start_ind) {
  for(int i=start_ind; i<(int)dir.length(); i++ ) {
    if(dir[i] == '/') {
      return i;
    }
  }
  return -1;
}

/*
- get all the names from the directory string
- return the number of names it read
*/
int GetDirectoryNames(string dir, string* names) {
  int start = 0;
  int curr_name_ind = 0;

  while(start < (int)dir.length()) {

    // if(dir == "/") {
    //   return 1;
    // }
    
    int start_slash_ind = FindSlashIndex(dir, start);
    int end_slash_ind = FindSlashIndex(dir, (start+1));
    // string curr_name = "";
    if(start_slash_ind > start) {
      names[curr_name_ind] = dir.substr((start), ((start_slash_ind)-start));
      start+=start_slash_ind;
      curr_name_ind += 1;

    } else if(start_slash_ind == ((int)dir.length()-1)) {
      start = dir.length();
  
    } else if ((start_slash_ind < 0) && (end_slash_ind < 0)) {
      names[curr_name_ind] = dir.substr(start, ((dir.length())-start));
      curr_name_ind+=1;
      start = dir.length();

    } else if (start_slash_ind == start) {
      start+=1;
      if(end_slash_ind < 0) {
        names[curr_name_ind] = dir.substr((start), ((dir.length())-start));
        start = dir.length();
    
      } else {
        names[curr_name_ind] = dir.substr((start), ((end_slash_ind)-start));
        start = end_slash_ind;
      }
      curr_name_ind += 1;

    }  
  }
  return curr_name_ind;
}

/*
- use to get size of array that holds the names
*/
int GetNumSlash(string dir) {
  int ctr = 0;
  int dir_ind = 0;
  int has_slash = FindSlashIndex(dir, dir_ind);
  while (has_slash >= 0) {
    dir_ind = has_slash+1;
    ctr+=1;
    has_slash = FindSlashIndex( dir, dir_ind);
  }
  return ctr;
}




/*
- assume dir_data is for sure data from a directory
*/
void DirFileNames(int inode_num, unsigned char* dir_data, int file_size, string* names){
  int byte = 0;
  int name_ind = 0;
  string curr_name = "";

  while(byte < file_size) {
    int entry_byte = byte%DIR_ENT_SIZE_BYTES;

    if(entry_byte < DIR_ENT_NAME_SIZE) {
      int curr_char = dir_data[byte];
      int invalid = 1;
      if((inode_num == 0) && (curr_name == ".") && (curr_char == '.')) {
        curr_name+=curr_char;
        names[name_ind] = curr_name;

        curr_name = "";
        invalid = -1;
        byte+=30;
        name_ind+=1;
      }
      if((curr_char != 0) && (invalid>0)) {
        curr_name += curr_char;
      }
      byte+=1;
    } else if(entry_byte == DIR_ENT_NAME_SIZE) {
      //name should be done now
      names[name_ind] = curr_name;
      
      curr_name = "";
      name_ind+=1;
      byte+=4;
    }
    
  }
  
}






/*
  Use this function with std::sort for directory entries
bool compareByName(const dir_ent_t& a, const dir_ent_t& b) {
    return std::strcmp(a.name, b.name) < 0;
}
*/
int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile directory\n" ;
    cerr << "For example:\n";
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img /a/b\n" ;
    return 1;
  }

  //----------------COMMAND LINE
  // parse command line arguments
  //  argv[0] = running this file
  //  argv[1] = file image thing to read disk from
  //  argv[2] = directory we want to print out
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  //file system gets disk
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  //directory  stores namme of directory
  string directory = string(argv[2]);

  //----------------GET DIR NAMES
  //get the names of each directory listed in "directory" string we're given
  int num_poss_dirs = GetNumSlash(directory);
  string dir_names[num_poss_dirs];
  int num_dir = GetDirectoryNames(directory, dir_names);
  //if empty, print root directory
  
   //----------------LOOKUPS
  //NEED TO HANDLE ERRORS IN LOOKUP
  int parent_inode = 0;
  int curr_name_ind = 0;
  while((parent_inode >= 0) && (curr_name_ind < num_dir)) {
    parent_inode = fileSystem->lookup(parent_inode, dir_names[curr_name_ind]);
    if(parent_inode>=0) {
      curr_name_ind+=1;
    } else {
      cerr << "Directory not found\n";
      delete fileSystem;
      delete disk;
      return 1;
    }
  }
  if(parent_inode < 0) {
    cerr << "Directory not found\n";
    delete fileSystem;
    delete disk;
    return 1;
  }
  
  //----------------GET INODE
  //should now have the correct current inode number
  //parent_inode = inode # of current inode we now want to print out 
  //curr_inode = inode we're looking at
  inode_t curr_inode;
  int stat_succ = fileSystem->stat(parent_inode, &curr_inode);
  if(stat_succ < 0) {
    cerr << "Directory not found\n";
    delete fileSystem;
    delete disk;
    return 1;
  }

  //----------------PRINTING
  int file_size = curr_inode.size;
  unsigned char buff[file_size];
  int read_in = fileSystem->read(parent_inode, buff, file_size );
  if(file_size != read_in) {
    cerr << "error with read";
    delete fileSystem;
    delete disk;
    return 1;
  }

  
  if(curr_inode.type == UFS_DIRECTORY) {

    int num_entries = curr_inode.size/DIR_ENT_SIZE_BYTES;
    string names[num_entries];
    DirFileNames(parent_inode, buff, file_size, names);
    sort(names, (names+num_entries));
    
    int inode_nums[num_entries];
    for(int i=0; i<num_entries; i++) {
      inode_nums[i] = fileSystem->lookup(parent_inode, names[i]);
      cout<<inode_nums[i]<<"\t"<<names[i]<<"\n";
    }
    
  } else if(curr_inode.type == UFS_REGULAR_FILE) {
    //4.in = argv[2] = "/a/b/c.txt"
    // cout<<argv[2];
    cout<<parent_inode<<"\t"<<dir_names[(num_dir-1)]<<"\n";
    // for(int i=0; i<file_size; i++) {
    //   //issue here? nnot supposed to print file contents?
    //   
    //   cout<<buff[i];
    // }
  }
  
  delete fileSystem;
  delete disk;

  return 0;
}





