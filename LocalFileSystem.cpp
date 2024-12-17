#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <cstring>

#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;

#define INT_SIZE (4)
#define DIR_ENT_SIZE_BYTES  (DIR_ENT_NAME_SIZE + INT_SIZE)

super_t super_block = super_t();

LocalFileSystem::LocalFileSystem(Disk *disk) {
  this->disk = disk;

}

/*
assume blocks have data put in sequentailly?
readSuperBlock(&super_block);   
*/
void LocalFileSystem::readSuperBlock(super_t *super) {
  unsigned char block_buff[UFS_BLOCK_SIZE];
  disk->readBlock(0, block_buff);
 
   for(int byte=0; byte<UFS_BLOCK_SIZE; byte++) {
    if((byte+1)%4 == 0) {
      
      int curr_int = int((block_buff[(byte)]) << 24 | (block_buff[(byte-1)]) << 16 | (block_buff[(byte-2)]) << 8 | (block_buff[byte-3]));
      switch(byte) {
        case(INT_SIZE-1):
          super->inode_bitmap_addr = curr_int;
          break;
        case((INT_SIZE*2)-1):
          super->inode_bitmap_len = curr_int;
          break;
        case((INT_SIZE*3)-1):
          super->data_bitmap_addr = curr_int;
          break;
        case((INT_SIZE*4)-1):
          super->data_bitmap_len = curr_int;
          break;
        case((INT_SIZE*5)-1):
          super->inode_region_addr = curr_int;
          break;
        case((INT_SIZE*6)-1):
          super->inode_region_len = curr_int;
          break;
        case((INT_SIZE*7)-1):
          super->data_region_addr = curr_int;
          break;
        case((INT_SIZE*8)-1):
          super->data_region_len = curr_int;
          break;
        case((INT_SIZE*9)-1):
          super->num_inodes = curr_int;
          break;
        case((INT_SIZE*10)-1):
          super->num_data = curr_int;
          break;
      }
    }

    
  }

}


 /*   SHOULD BE WORKING
  - super = super block, has info about bitmap
  - get addr (in blocks) and len (in blocks) of bitmap
  - for each block, read from disk and put in bitmap pointer (to be returned) 
  - everything in bytes: 
      bitmap = char = byte indices
      block size = 4096 bytes 
      (update pointer to add things to the bitmap by block size)
  */
void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  int inode_bm_addr = super->inode_bitmap_addr;
  //inode_bm_len*UFS_BLOCK_SIZE = # bytes used in inode bitmap
  int inode_bm_len = super->inode_bitmap_len;
  //num_inodes = number of bits we use for the bitmap
  //bitmap = char = 1 byte chunks = 8 bits; 32bits = 4 bytes = 4 char in bitmap
  int num_inodes = super->num_inodes; 
  int num_bytes_toread = num_inodes/8;
  int leftover_bits = num_inodes%8;

  int blocks_read = 0;
  //block size in bytes; 1 byte = 1 char =  1 index in bitmap
  //1 block = 4096 bytes = 4096 char
  //UFS_BLOCK_SIZE
  //curr_block used to update bitmap, inodeBitmap still points to the start of the char arr
  unsigned char curr_block[UFS_BLOCK_SIZE];    //in bytes
  while(blocks_read < inode_bm_len) {
    //assumes bm blocks are next to each other, probably won't ever use this loop
    disk->readBlock((inode_bm_addr), curr_block);
    // curr_block += UFS_BLOCK_SIZE;
    inode_bm_addr+=1;
    blocks_read+=1;

    for(int byte = 0; byte<num_bytes_toread; byte++) {
      inodeBitmap[byte] = curr_block[byte];
    }
    if(leftover_bits!=0) {
      cout<<"readInodeBitmap error?";
    }
  }
  
}

/*
- assume bitmap is already in the correct format
- assume super block already updated?
*/
void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  //both in blocks
  int inode_bm_addr = super->inode_bitmap_addr;
  int inode_bm_len = super->inode_bitmap_len;
  
  //get size of curr bm
  int num_inode = super->num_inodes;
  int bm_num_bytes = num_inode/8;
  // if((num_inode%8) != 0)  {
  //   bm_num_bytes+=1;    //+1 = 8 additional bits of space added 
  // }

  unsigned char curr_block[UFS_BLOCK_SIZE];    //in bytes
  unsigned char prev_bm[UFS_BLOCK_SIZE];
  int blocks_read = 0;
  while(blocks_read < inode_bm_len) {
    //assumes bm blocks are next to each other, probably won't ever use this loop
    disk->readBlock(inode_bm_addr, curr_block);
    inode_bm_addr+=1;
    blocks_read+=1;

    if(bm_num_bytes < UFS_BLOCK_SIZE) {
      //NEED TO CHECK BM SIZEE????
      //for each byte in the bm, store in the prev_bm
      for(int byte=0; byte<bm_num_bytes; byte++) {
        if(curr_block[byte] == inodeBitmap[byte])  {
          prev_bm[byte] = curr_block[byte];
        } else {
          prev_bm[byte] = inodeBitmap[byte];
        }
        
      }
    }

    disk->writeBlock((inode_bm_addr-1), prev_bm);
    //bm should be inn char and donne now
  }

}


/*    NEED TO FIX?
  - bit map to represent which blocks on disk are being used
*/
void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {

  int data_bm_addr = super->data_bitmap_addr;
  int data_bm_len = super->data_bitmap_len;   //num blocks used for bm

  int num_data_blks = super->num_data;
  int bm_byte_size = num_data_blks/8;

  int blocks_read = 0;

  unsigned char curr_block[UFS_BLOCK_SIZE];
  while(blocks_read < data_bm_len) {
    disk->readBlock(data_bm_addr, curr_block);    //in bytes
    
    data_bm_addr+=1;
    blocks_read+=1;

    for(int byte=0; byte<bm_byte_size; byte++) {
      dataBitmap[byte] = curr_block[byte];
    }
  }
}



/*
-  copying writeInodeBitMap
*/
void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {
   //both in blocks
  int data_bm_addr = super->data_bitmap_addr;
  int data_bm_len = super->data_bitmap_len;
  
  //get size of curr bm
  int num_data = super->num_data;
  int bm_num_bytes = num_data/8;
  // if((num_data%8) != 0)  {
  //   bm_num_bytes+=1;    //+1 = 8 additional bits of space added 
  // }

  unsigned char curr_block[UFS_BLOCK_SIZE];    //in bytes
  unsigned char prev_bm[UFS_BLOCK_SIZE];
  int blocks_read = 0;
  while(blocks_read < data_bm_len) {
    //assumes bm blocks are next to each other, probably won't ever use this loop
    disk->readBlock(data_bm_addr, curr_block);
    data_bm_addr+=1;
    blocks_read+=1;

    if(bm_num_bytes < UFS_BLOCK_SIZE) {
      //NEED TO CHECK BM SIZEE????
      //for each byte in the bm, store in the prev_bm
      for(int byte=0; byte<bm_num_bytes; byte++) {
        if(curr_block[byte] == dataBitmap[byte])  {
          prev_bm[byte] = curr_block[byte];
        } else {
          prev_bm[byte] = dataBitmap[byte];
        }
        
      }
    }

    disk->writeBlock((data_bm_addr-1), prev_bm);
    //bm should be inn char and donne now
  }
}





/* 
- *inodes = inode table that stores all the inodes; only add values to the pointer, 
  array holding the values doesn't exist in this function
  - static nodes lets us add data  to the table and not lose it after function ends?

- assume all the blocks allocated to inode region are filled with evenly
  sized inodes = calculate inode size
- assume num_inodes is the max number of inodes we can have, not the 
  current count of inodes being used (that's why there's an inode bitmap)
- inode = 128 bytes? int=4bytes, type = 1 int, size = 1 int, direct max 
  size = 30 * 1int
    30+2 = 32 ints * 4bytes/1int = 32*4= 128bytes per inode
    4096bytes/block / 128bytes/inode = 32 inodes per block
- assume inode data is sequentially put in inode region?
  1st 4bytes = type, 2nd 4bytes = size, 30 remaining bytes = direct???
  least significant bits = first bits??
- direct = holds block addresses of the data that backs the file
*/
void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
  int inode_reg_addr = super->inode_region_addr;  //in blocks
  int inode_reg_len = super->inode_region_len;    //in blocks
  // int nm_inodes = super->num_inodes;

  int inode_size_bytes = 128;  //128 bytes
  //inode bitmap tells us which inodes are available
  // int inode_per_blk = UFS_BLOCK_SIZE/128;
  

  int blocks_read = 0;
  unsigned char curr_block[UFS_BLOCK_SIZE];

  int inode_ctr = 0;

  while(blocks_read < inode_reg_len) {
    disk->readBlock(inode_reg_addr, curr_block);
    inode_reg_addr+=1;
    blocks_read+=1;

    int direct_ind = 0;
    //static so the data doesn't get destroyed
    static inode_t curr_node;

    for(int byte=0; byte<UFS_BLOCK_SIZE; byte++) {
      //curr_inode_byte index starts at 0
      int curr_inode_byte = (byte)%inode_size_bytes;
      
      if((curr_inode_byte+1)%4 == 0) {
        int curr_int = int((curr_block[(byte)]) << 24 | (curr_block[(byte-1)]) << 16 | (curr_block[(byte-2)]) << 8 | (curr_block[byte-3]));
        switch((curr_inode_byte)) {
          case(INT_SIZE-1):
            curr_node.type = curr_int;
            break;
          case((INT_SIZE*2)-1):
            curr_node.size = curr_int;
            break;
          default:
            curr_node.direct[direct_ind] = curr_int;
            direct_ind+=1;
        }
      }
      //if at the last byte of the inode, put inode  at inode table ptr
      if( curr_inode_byte == (inode_size_bytes-1)) {
        inodes[inode_ctr] = curr_node;
        inode_ctr += 1;   
        direct_ind = 0;   //reset the direct array index for the next inode
      }
    } 
  }
}

/*
- assume inodees param is the correct size
*/
void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {
  int inode_reg_len = super->inode_region_len;
  int inode_reg_addr = super->inode_region_addr;
  int num_inode = super->num_inodes;    //128 bytes each

  int inode_per_blk = num_inode/inode_reg_len;
  int inode_size_bytes = UFS_BLOCK_SIZE/inode_per_blk;  //128 bytes
  
  unsigned char block_towrite[UFS_BLOCK_SIZE];
  int block_byte = 0;
  int  num_blks_towrite = 0;

  for(int inode=0; inode<num_inode; inode++) {
    //go through whole table of inodes
    inode_t curr_inode = inodes[inode];
    
    //extract data from inode and store in block_towrite
    int curr_type = curr_inode.type;
    int curr_size = curr_inode.size;

  
    //TYPE--------------------------
    memcpy((block_towrite+block_byte), &curr_type, sizeof(int));
    //fill the rest with 0's 
    int type_num_bytes = 0;
    while(curr_type>0) {
      type_num_bytes += 1;
      curr_type /= 255;
    }
    while(type_num_bytes<4) {
      block_towrite[(block_byte+type_num_bytes)] = 0;
      type_num_bytes += 1;
    }
    block_byte+=sizeof(int);
    
    //SIZE--------------------------
    memcpy((block_towrite+block_byte), &curr_size, sizeof(int));
    //fill the rest with 0's 
    int size_num_bytes = 0;
    while(curr_size>0) {
      size_num_bytes += 1;
      curr_size /= 255;
    }
    while(size_num_bytes<4) {
      block_towrite[(block_byte+size_num_bytes)] = 0;
      size_num_bytes += 1;
    }
    block_byte+=sizeof(int);

    //DIRECT--------------------------
    //size of direct = based on size of file
    int num_direct_entries = (curr_inode.size)/UFS_BLOCK_SIZE;
    if(((curr_inode.size)%UFS_BLOCK_SIZE)  > 0) {
      num_direct_entries += 1;
    }
    for(int i=0; i<num_direct_entries; i++) {
      int direct_entry = curr_inode.direct[i];
      int dir_ind = block_byte+(sizeof(int)*i);
      memcpy((block_towrite+dir_ind), &direct_entry, sizeof(int));
    }
    //-8 since we alraedy copied over type and size = 2*4bytes
    block_byte += (inode_size_bytes-8);
  
    if(block_byte == UFS_BLOCK_SIZE) { 
      
      disk->writeBlock((inode_reg_addr+num_blks_towrite), block_towrite);
      num_blks_towrite += 1;
      for(int i=0; i<UFS_BLOCK_SIZE; i++) {
        block_towrite[i] = 0;
      }
    }
    
  }

}


/*  //--------------------SHOULD BE WORKING--------------------------
- assume super_block has been read in
- assume inodeNumber = index of inode in inode table?
- assumme directories immediately have directory info(32 byte chunks) in 
  inode direct array
- need to make array of num_inodes size, popuulate it with inodes, and use the info
  - arrays don't exist outside of the function

- ignore inode bitmap value; check for error elsewhere
- ROOT DIR = INODE_TABLE[0]
*/
int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  readSuperBlock(&super_block);

  //--------------------BITMAP
  //each bit in inode bitmap tells you num inodes we can use
  //ex = inode BM = 15 = 1111 = first 4 inodes are useable
  int inode_BM_size = (super_block.num_inodes)/8;
  if((super_block.num_inodes)%8 > 0) {
    inode_BM_size += 1;
  }
  unsigned char inode_BM[inode_BM_size];
  readInodeBitmap(&super_block, inode_BM);

  //--------------------INODE TABLE
  //inode table = ptr to start of array of inodes
  int num_inodes = super_block.num_inodes;
  inode_t inode_table[num_inodes];
  //fill in inode table
  readInodeRegion(&super_block, inode_table);

  //--------------------CHECK PARENTINODE VALIDITY IN BITMAP
  //assume parentInodeNumber = index of the inode as it's put into the inode table?
  //check bitmap to see if the parentInode is an inode being used rn
  //parent inode # = index
  int byte_ind = parentInodeNumber/8;
  int bit_ind = parentInodeNumber%8;
  int ind = 0;
  //make ind = byte in we want in bitmap
  for(int byte = 0; byte<byte_ind; byte++) {
    ind+=1;
  }
  //get the byte in decimal
  int dec_byte = inode_BM[ind];
  int msb = 1;
  int bits = 0;
  //convert byte in dec to  bits
  while(dec_byte>0) {
    int rem = dec_byte%2;
    bits += (rem * msb);
    msb *= 10;
    dec_byte = dec_byte/2;
  }
  //get bit wewant
  for(int bit=0; bit<bit_ind; bit++) {
    bits /= 10;
  }
  int bit = bits%10;

  if((parentInodeNumber >= num_inodes) || (bit == 0)) {
    //invalid inode number, out of bounds   EINVALIDINODE
    return -EINVALIDINODE;
  }
  inode_t parent_inode = inode_table[parentInodeNumber];

  if(parent_inode.type != UFS_DIRECTORY) {
    //invalid inode number, file not a directory    EINVALIDINODE
    return -EINVALIDINODE;
  }

//--------------------READ ENTRIES
  int file_size = parent_inode.size;

  //double checking that the bytes fit the directory type shape
  int wrong_num_bytes = file_size%DIR_ENT_SIZE_BYTES;
  if(wrong_num_bytes !=0 ) {
    return -EINVALIDINODE;
  } 
  
  //number of full blocks the directory takes up
  int dir_num_blks = file_size/UFS_BLOCK_SIZE;
  //number of bytes in an unfilled block that directory uses
  int dir_extra_bytes = file_size%UFS_BLOCK_SIZE;
  //total num blocks to read in; full blocks + overflow block
  int blks_need_to_read = dir_num_blks;
  if( (dir_extra_bytes > 0)) {
    blks_need_to_read+=1;
  }
  
  for(int curr_blk=0; curr_blk < blks_need_to_read; curr_blk++) {
    //get the block that is holding entry data
    int blk_w_entry = parent_inode.direct[(curr_blk)];

    unsigned char entries[UFS_BLOCK_SIZE];
    disk->readBlock(blk_w_entry, entries);

    int inside_blk_ind = 0;
    string entry_name = "";
   
    while(inside_blk_ind<UFS_BLOCK_SIZE) {
      int entry_ind = inside_blk_ind%DIR_ENT_SIZE_BYTES;

      //if reading in name bytes
      if(entry_ind<DIR_ENT_NAME_SIZE) {
        char name_char = entries[inside_blk_ind];

        //root directory will have invalid .. bytes; just make =0
        int invalid = 1;
        if(parentInodeNumber == 0) {
          if((entry_name == ".") && (name_char == '.')) {
            entry_name+=name_char;
            if(entry_name == name) {
              return 0;
            }
            //skip to next entry, clear entry name
            inside_blk_ind+=30;
            invalid = -1;
            entry_name = "";
          }
        }
        if((name_char != 0) && (invalid>0)) {
          entry_name += name_char;
        }

      //if done reading name
      } else if(entry_ind == DIR_ENT_NAME_SIZE) {
        if(entry_name != name ) {
          //skip to next entry, ind of firs char in next entry
          //makes sure to skip the last else if statement 
          inside_blk_ind+=3;
          entry_name = "";
        }

      //get inode # and return 
      } else if(entry_ind == (DIR_ENT_SIZE_BYTES-1)) {
        if (entry_name==name) {
          return (int((entries[(inside_blk_ind)]) << 24 | (entries[(inside_blk_ind-1)]) << 16 | (entries[(inside_blk_ind-2)]) << 8 | (entries[inside_blk_ind-3])));
        }
      }

      inside_blk_ind+=1;
    }
  }
  return -ENOTFOUND;
}




/*
- inode = inode we want to return
- EINVALIDINODE= only error type, otherwise return 0 success
*/
int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  //not sure if i need to read in super block??
  // need this for ds3cat
  readSuperBlock(&super_block);

  if((inodeNumber < 0) || (inodeNumber > super_block.num_inodes) ) {
    return -EINVALIDINODE;
  }

  //--------------------BITMAP
  //each bit in inode bitmap tells you num inodes we can use
  //ex = inode BM = 15 = 1111 = first 4 inodes are useable
  int inode_BM_size = (super_block.num_inodes)/8;
  unsigned char inode_BM[inode_BM_size];
  readInodeBitmap(&super_block, inode_BM);

  //--------------------BITMAP FOR BOUNDS
  //get mapping to match the nodes by converting to bits

  int byte_ind = inodeNumber/8;
  int bit_ind = inodeNumber%8;
  int ind = 0;
  //make ind = byte in we want in bitmap
  for(int byte = 0; byte<byte_ind; byte++) { 
    ind+=1;
  }
  //get the byte in decimal
  int dec_byte = inode_BM[ind];
  int msb = 1;
  int bits = 0;
  //convert byte in dec to  bits
  while(dec_byte>0) {
    int rem = dec_byte%2;
    bits += (rem * msb);
    msb *= 10;
    dec_byte = dec_byte/2;
  }
  //get bit wewant
  for(int bit=0; bit<bit_ind; bit++) {
    bits /= 10;
  }
  int bit = bits%10;

  int num_inodes = super_block.num_inodes;
  if((inodeNumber >= num_inodes) || (bit == 0)) {
    //invalid inode number, out of bounds   EINVALIDINODE
    return -EINVALIDINODE;
  }

  //-------------------- FILL INODE TABLE
  //fill in inode table
  inode_t inode_table[num_inodes];
  readInodeRegion(&super_block, inode_table);

  inode_t curr_inode = inode_table[inodeNumber];
  inode->type = curr_inode.type;
  inode->size = curr_inode.size;
  for(int i=0; i<DIRECT_PTRS; i++) {
    inode->direct[i] = curr_inode.direct[i];
  }
  
  return 0;
}



/*
- size in bytes to read 
- file type = buffer holds file contents in bytes (unsigned char array)
- directory type = buffer holds same? Just also has more restrictions on 
  size? 
*/
int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {

  // readSuperBlock(&super_block);

  //--------------------GET CURR INODE
  //deals with invalid inodes
  inode_t curr_inode;
  int stat_succ = stat(inodeNumber, &curr_inode);
  if(stat_succ != 0) {
    return -EINVALIDINODE;
  }
  //trying to read more data than the inode contains
  if(curr_inode.size < size) {
    return -EINVALIDSIZE;
  }

  //--------------------READING CONTENTS
  int file_size = curr_inode.size;
  int num_full_blocks = file_size/UFS_BLOCK_SIZE;
  int overflow_bytes = file_size%UFS_BLOCK_SIZE;

  int blks_need_to_read = num_full_blocks;
  if( (overflow_bytes > 0)) {
    blks_need_to_read+=1;
  }
  //curr_block stores the current block data we read in
  unsigned char curr_block[UFS_BLOCK_SIZE];
  unsigned char temp_buff[size];
  int temp_b_ind = 0;
  unsigned char* temp_buff_loc = &temp_buff[temp_b_ind];

  for(int blk_num = 0; blk_num < blks_need_to_read; blk_num++) {
    int blk_ind = curr_inode.direct[blk_num];
    disk->readBlock(blk_ind, curr_block);

    //read partial block into temp buffer
    if((blk_num == (blks_need_to_read-1)) && (overflow_bytes > 0)) {
      memcpy(temp_buff_loc, curr_block, overflow_bytes);
      temp_b_ind += overflow_bytes;
      temp_buff_loc += overflow_bytes;

    //read entire block into temp buffer
    } else {
      memcpy(temp_buff_loc, curr_block, UFS_BLOCK_SIZE);
      temp_b_ind += UFS_BLOCK_SIZE;
      temp_buff_loc += UFS_BLOCK_SIZE;

    }

    if(temp_b_ind == size) {
      memcpy(buffer, temp_buff, size);
      return size;
    }
    
  }
  
  //--------------------RETURN
  return -1;


}



/*
- parent = directory we want to make the thing  in
- MAKE FUNCTIONN TO CHECK BITMAP  AMMOUNT OF OPEN INNDICES
*/
int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  /*
  - bits max size is 1111 1111
  */
  // auto CharToBinary = [](int dec_char) {
  //   if((dec_char > 255) || (dec_char < 0)) {
  //     return -1;
  //   }
  //   int msb = 1;
  //   int bits = 0;
  //   //convert byte in dec to bits
  //   while(dec_char>0) {
  //     int rem = dec_char % 2;
  //     bits += (rem * msb);
  //     msb *= 10;
  //     dec_char /= 2;
  //   }
  //   return bits;
    
  // };

  auto NumBlksForSize = [](int size) {
    int num_blks = size/UFS_BLOCK_SIZE;
    if(size%UFS_BLOCK_SIZE > 0) {
      num_blks+=1;
    }
    return num_blks;
  };


  //--------------------------------------------------------------------------------------------------------
  //--------------------------------------------------------------------------------------------------------
  /*
  - reeturns block  ind of open  space, relative to the start of that block type's reegion
  */
  auto FirstAvailSpace_BM = [](unsigned char* bm, int bm_len) { 
    //find opeen space in bm
    int open_space_ind = -1;
    int byte = 0;
    while(byte<bm_len) {
      if(bm[byte] != 255) {
        open_space_ind = byte;
        byte = bm_len;
      }
      byte++;
    }
    if(open_space_ind < 0) {
      return -ENOTENOUGHSPACE;
    }
    //---------------------------ISOLATE BIT
    //get specific available bit=0 that represents available block for data
    int dec_byte = bm[open_space_ind];
    int msb = 1;
    int bits = 0;
    //convert byte in dec to bits
    while(dec_byte>0) {
      int rem = dec_byte%2;
      bits += (rem * msb);
      msb *= 10;
      dec_byte /= 2;
    }
    //get bit wewant
    int block_ind= open_space_ind*8;
    int new_bm_byte = bits;
    int mult = 1;
    while((bits%10) != 0) {
      block_ind += 1;
      mult *= 10;
      bits /= 10;
    }

    //---------------------------WRITE TO BM
    new_bm_byte+=mult;
    //convert to dec
    int new_bm_dec = 0;
    mult = 1;
    while(new_bm_byte>0) {
      int curr_bit = new_bm_byte%10;
      new_bm_dec += curr_bit*mult;
      mult*=2;
      
      new_bm_byte /= 10;
    }
    bm[open_space_ind] = new_bm_dec;
    
    return block_ind;
  };
  //--------------------------------------------------------------------------------------------------------
  //--------------------------------------------------------------------------------------------------------

  //--------------------BEGINNING ERROR CHECK - name length
  //name too long = invalid name
  if(name.length() > DIR_ENT_NAME_SIZE) {
    return -EINVALIDNAME;
  }

  //--------------------GET PARENT INODE
  inode_t parent_inode;
  int stat_succ1 = stat(parentInodeNumber, &parent_inode);
  //parent inode not found
  if(stat_succ1!=0) {
    return  -EINVALIDINODE;
  }
  
  //--------------------PARENT ERROR CHECK
  //parent not directory
  if(parent_inode.type != UFS_DIRECTORY) {
    return -EINVALIDTYPE;
  }
  

  //--------------------CHECK NAME EXISTS
  //has_name = also represents inode# of file we want to make
  int has_name = lookup(parentInodeNumber, name);
  //if name exists, get existing  file, check type
  if(has_name > 0) {
    inode_t existing_file;
    int stat_succ2 = stat(has_name, &existing_file);

    //stat_succ = existing_file holds correct info
    if(stat_succ2 == 0) {
      if(existing_file.type ==  type) {
        return 0;
      } else {
        return -EINVALIDTYPE;
      }
    }
  }


  //--------------------------------------------------------------------------------
  //--------------------MAKE FILE OTHERWISE--------------------
  //--------------------BM AND DATA VARS - FOR REUSE
  //inode bm
  int inode_bm_len = ((super_block.num_inodes)/8);
  unsigned char inode_bm[inode_bm_len];

  //inode table = ptr to start of array of inodes
  int num_inodes = super_block.num_inodes;
  inode_t inode_table[num_inodes];

  //data bm
  int data_bm_len = (super_block.num_data)/8;
  unsigned char data_bm[data_bm_len];

  //both start as 1 sincee makinng a new inode 
  //neew inode -> innode table, also data regionn to support innode
  int num_data_blks_need  = 1;
  // int num_inode_blks_need =  1;

  //--------------------READ INODE BITMAP
  readInodeBitmap(&super_block, inode_bm);
  // unsigned char fix_inode_bm[inode_bm_len];
  // readInodeBitmap(&super_block, fix_inode_bm);

  //--------------------READ INODE TABLE
  readInodeRegion(&super_block, inode_table);

  //--------------------READ DATA BITMAP
  readDataBitmap(&super_block, data_bm);


  //--------------------CHECK IF PARENT NEEDS ANOTHER BLK
  //check if adding directory to  parent inode would incraese number of blocks (change parent's direct arr)
  int curr_parent_size = parent_inode.size;
  if( NumBlksForSize(curr_parent_size) < NumBlksForSize((curr_parent_size+DIR_ENT_SIZE_BYTES)) ){
    //need a new block in  data region to hold parent data
    num_data_blks_need += 1;
  }

  //--------------------CHECK for space in inode BM
  int byte = 0;
  while(byte<inode_bm_len) {
    if(inode_bm[byte] != 255) {
      break;
    }
    byte+=1;
  }
  if(byte == (inode_bm_len-1))  {
    //no space
    return -ENOTENOUGHSPACE;
  }
   
  //--------------------CHECK for space in data BM
  for(int i=0; i<data_bm_len; i++) {
    if(data_bm[i] < 255) {
      num_data_blks_need -= 1;
      if(data_bm[i] < 252) {
        num_data_blks_need -= 1;
      }
    }
  }
  if(num_data_blks_need > 0) {
    return -ENOTENOUGHSPACE;
  }
 
  //--------------------WRITE INODE BM
  int inode_table_ind = FirstAvailSpace_BM(inode_bm, inode_bm_len);
  if(inode_table_ind == (-ENOTENOUGHSPACE)) {
    return -ENOTENOUGHSPACE;
  }
  writeInodeBitmap(&super_block, inode_bm);

  //--------------------MAKE FILE DIR ENTRY 
  // dir_ent_t new_file;
  unsigned char dir_ent_data[UFS_BLOCK_SIZE];
  for(int i=0; i<(int)name.length(); i++) {
    dir_ent_data[i] = name[i];
    // new_file.name[i] = name[i];
  }
  // new_file.inum = inode_table_ind;
  // int dir_ent_ind = ;
  //convert int to char bytes (4)
  int inum = inode_table_ind;
  memcpy((dir_ent_data+DIR_ENT_NAME_SIZE), &inode_table_ind, sizeof(int));
  
  int num_bytes_used = 0;
  while(inum>0) {
    num_bytes_used +=1;
    inum /= 255;
  }
  //fill in the rest of the bytes in dir ent data
  while(num_bytes_used < 4) {
    dir_ent_data[(DIR_ENT_NAME_SIZE+num_bytes_used)] = 0;
    num_bytes_used += 1;
  }

  //--------------------ADD DIRECTORY ENTRY TO PARENT
  // //update parent inode size
  parent_inode.size += DIR_ENT_SIZE_BYTES;
  int num_blocks= (parent_inode.size)/UFS_BLOCK_SIZE;

  //if adding directory entry to parent causes a new block, update direct array in parent
  if(((parent_inode.size)%UFS_BLOCK_SIZE) == DIR_ENT_SIZE_BYTES) {

    //find next open block = check data bitmap
    readDataBitmap(&super_block, data_bm);
    int open_data_bm = FirstAvailSpace_BM(data_bm, data_bm_len);
    if(open_data_bm == (-ENOTENOUGHSPACE)) {
      //FIX ALL WRITES BEFORE - fix write innode bm
      // writeInodeBitmap(&super_block, fix_inode_bm);
      return -ENOTENOUGHSPACE;
    }
    unsigned char fix_data_bm[data_bm_len];
    readDataBitmap(&super_block, fix_data_bm);
    writeDataBitmap(&super_block, data_bm);

    parent_inode.direct[num_blocks]= (open_data_bm + (super_block.data_bitmap_addr)); 
  } 

  //--------------------WRITE DIRECTORY ENTRY DATA 
  //otherwise/after add data to the end of the curr block, and write to disk
  unsigned char data_blk[UFS_BLOCK_SIZE];
  disk->readBlock((parent_inode.direct[num_blocks]), data_blk);
  int last_ind = ((parent_inode.size) % UFS_BLOCK_SIZE)  - DIR_ENT_SIZE_BYTES;
  memcpy((data_blk+last_ind), dir_ent_data, DIR_ENT_SIZE_BYTES);
  disk->writeBlock((parent_inode.direct[num_blocks]), data_blk);
  


  //--------------------MAKE INODE 

  //direct doesn't exist yet?
  inode_t curr_inode = {.type = type, .size=0};
  if(type == UFS_DIRECTORY) {
    curr_inode.size = 2*DIR_ENT_SIZE_BYTES;

    //already did inode bm, need to update inode table?

    //make .(curr) and ..(parent) entries
    unsigned char ent_data[UFS_BLOCK_SIZE];    
    //name = . 
    //inode # = inode_table_ind
    ent_data[0] = '.';
    for(int i=1; i<DIR_ENT_NAME_SIZE; i++ ) {
      ent_data[i] = 0;
    }
    memcpy((ent_data+DIR_ENT_NAME_SIZE), &inode_table_ind, sizeof(int));

    //name = ..
    //inode # = parentInodeNumber
    ent_data[DIR_ENT_SIZE_BYTES] = '.';
    ent_data[(DIR_ENT_SIZE_BYTES+1)] = '.';
    for(int i=2; i<DIR_ENT_NAME_SIZE; i++ ) {
      ent_data[(DIR_ENT_SIZE_BYTES+i)] = 0;
    }
    memcpy(((ent_data+DIR_ENT_NAME_SIZE)+DIR_ENT_SIZE_BYTES), &parentInodeNumber, sizeof(int));

    //add to new data block
    readDataBitmap(&super_block, data_bm);
    int data_bm_space = FirstAvailSpace_BM(data_bm, data_bm_len);
    if(data_bm_space == (-ENOTENOUGHSPACE)) {
      //FIX ALL WRITES BEFOREE
      return -ENOTENOUGHSPACE;
    }
    writeDataBitmap(&super_block, data_bm);
    int ent_blk_ind = (data_bm_space +(super_block.data_region_addr));
    curr_inode.direct[0] = ent_blk_ind;
    disk->writeBlock(ent_blk_ind, ent_data);

  //otherwise want to makee a file
  }   else  {
    readDataBitmap(&super_block, data_bm);
    int data_bm_space = FirstAvailSpace_BM(data_bm, data_bm_len);
    if(data_bm_space == (-ENOTENOUGHSPACE)) {
      //FIX ALL WRITES BEFOREE
      return -ENOTENOUGHSPACE;
    }
    writeDataBitmap(&super_block, data_bm);
    int ent_blk_ind = (data_bm_space +(super_block.data_region_addr));
    curr_inode.direct[0] = ent_blk_ind;
    // disk->writeBlock(ent_blk_ind, ent_data);

  }

  //--------------------ADD INODE TO TABLE
  inode_table[parentInodeNumber] = parent_inode;
  inode_table[inode_table_ind]  = curr_inode;
  writeInodeRegion(&super_block, inode_table);

  // unsigned char check_bm[4096];
  // readInodeRegion(&super_block, inode_table);
  // readInodeBitmap(&super_block, check_bm);

  return 0;
}






/*
- assume we don't want it to work for an  inode that doesn't exist -  stat and EINVALIDINODE eerror
- replace file contents basically?
- NEEDTO MAKE RETURN VALUE THE NNUM BYTES  SUCCESSFULLY WRITTEN
*/
int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {

   //--------------------------------------------------------------------------------------------------------
  //--------------------------------------------------------------------------------------------------------
  /*
  - ind of bm with space on success,  -1 if no space
  - updates bm_byte to  hold the char of the bm with space
  - start_ind = index of bitmap
  */
  auto AvailSpace_BM = [](unsigned char* bm, int bm_len, int start_ind, unsigned char* bm_byte) { 
    for(int i=start_ind; i<bm_len; i++) {
      if(bm[i] != 255) {
        *bm_byte = bm[i];
        return i;
      }
    }
    return -1;
  };
  //--------------------------------------------------------------------------------------------------------
  //--------------------------------------------------------------------------------------------------------
  

  //read in inode 
  inode_t curr_inode;
  int stat_succ = stat(inodeNumber, &curr_inode);
  if(stat_succ!=0) {
    return -EINVALIDINODE;
  }
  //--------------------CHECK FILE TYPE
  if(curr_inode.type != UFS_REGULAR_FILE) {
    return -EINVALIDTYPE;
  }

  //--------------------CHECK SIZES
  if((size>MAX_FILE_SIZE) || (size<0)) {
    return -EINVALIDSIZE;
  }
  int num_blks = size/UFS_BLOCK_SIZE;
  if(((size)%UFS_BLOCK_SIZE) > 0) {
    num_blks +=1;
  }
  
  int old_num_blks = (curr_inode.size)/UFS_BLOCK_SIZE;
  if(((curr_inode.size)%UFS_BLOCK_SIZE) > 0) {
    old_num_blks +=1;
  }

  //keeep track of what to change
  int change_inode_size = -1;
  //num  blocks to add to the direct arr
  int add_direct_arr_ent  = -1;
  int change_data_bm  = -1;

  int data_bm_size = (super_block.num_data)/8;
  unsigned char data_bm[data_bm_size];
  readDataBitmap(&super_block, data_bm);

  if(size == curr_inode.size) {
    //don't need to change inode details
      //don't need to change direct array
    //don't need to change data bitmap
    //write block data directly to disk
    
  } else if(num_blks == old_num_blks) {
    //need to change inode details - change inode table = write to inode region
      //size
      //don't need to change direct array
    //don't need to change data bitmap
    change_inode_size = 1;
    // int overflow = size%UFS_BLOCK_SIZE;
    // int curr_overflow = (curr_inode.size)%UFS_BLOCK_SIZE;

  } else {
    //if (old_num_blks > num_blks)
    //need to change inode details - change inode table = write to inode region
      //size
      //don't need to change direct array since size will cut off the unused direect entry 
    //need to change data bitmap - write to data bitmap
    change_inode_size = 1;
    change_data_bm = 1;
    //if (old_num_blks < num_blks)
    //need to error check - data  bitmap
    //need to change inode details - change inode table = write to inode region
      //size
      //nneed to  add entry to direct array
    //need to change data bitmap - write to data bitmap
    add_direct_arr_ent = num_blks - old_num_blks;

  } 
  
  //--------------------CHANGE INODE DATA
  inode_t old_inode = curr_inode;

  if((change_inode_size > 0) || (add_direct_arr_ent > 0)) {
    //update size
    if(change_inode_size > 0) {
      curr_inode.size = size;
    } 

    //update direct array
    if(add_direct_arr_ent > 0)  {
      //keep track of data blocks we want to write, NNOT INCLUDING DATA REGION ADDR
      int blk_ind_towrite[(DIRECT_PTRS)];
      int num_indices = 0;

     
      unsigned char bm_byte = 0;
      int bm_ind = -1;

      //while there are open spaces in the bit map
      while((bm_ind < data_bm_size) && (add_direct_arr_ent>0)) {

        bm_ind = AvailSpace_BM(data_bm, data_bm_size, (bm_ind+1), &bm_byte);
        if(bm_ind < 0) {
          return -EINVALIDSIZE;
        }

        //while there are blocks to add to direct, and there is space in bm char
        while((bm_byte !=  255) && (add_direct_arr_ent>0)) {
          //------------------convert bm char to binary
          //get the byte in decimal
          int dec_byte = bm_byte;
          int msb = 1;
          int bits = 0;
          //convert byte in dec to  bits
          while(dec_byte>0) {
            int rem = dec_byte%2;
            bits += (rem * msb);
            msb *= 10;
            dec_byte = dec_byte/2;
          }
          //get block ind we want
          //bits_to_dec = used to set curr char to have new used block ind
          int convert_one = 1;
          int blk_ind = bm_ind*8;
          while(bits>0) {
            int lsb = bits%10;
            if(lsb == 1) {
              convert_one *= 2;
              blk_ind+=1;
            } else {
              
              break;
            }
            bits /= 10;
          }
          bm_byte += convert_one;

          //add block index to the list of data blocks we want to write
          blk_ind_towrite[num_indices] = blk_ind;
          num_indices += 1;
          add_direct_arr_ent -= 1;
        }
      }

      int last_dir_ent = (old_inode.size)/UFS_BLOCK_SIZE;
      if((old_inode.size)%UFS_BLOCK_SIZE > 0) {
        last_dir_ent += 1;
      }
      for(int i=0; i<num_indices; i++) {
        
        curr_inode.direct[(last_dir_ent+i)] = (blk_ind_towrite[i]) + (super_block.data_region_addr);
      }
    }

    //for both now, have to update  inode region - parent block
    //geet inode table, uupdate table, write inode table
    inode_t  inode_table[(super_block.num_inodes)];
    readInodeRegion(&super_block, inode_table);
    inode_table[inodeNumber] = curr_inode;
    writeInodeRegion(&super_block, inode_table);
  }
  
  if(change_data_bm>0)  {
    //if want to make the file smaller, free up the uused blks in data bm
    if(old_inode.size > curr_inode.size) {
      int num_blk_to_del = old_num_blks - num_blks;

      //geetthe blocks that direct used to point to, ABSOLUTE BLK IND
      for(int i=0; i<num_blk_to_del; i++) {
        int blk_ind_to_del = (old_inode.direct[(old_num_blks-1)]) - super_block.data_region_addr;
        old_num_blks -= 1;

        int full_bytes  = blk_ind_to_del/8;
        int bits_in = blk_ind_to_del%8;
        int curr_byte = data_bm[full_bytes];

        int set_zero = 1;
        for(int x=0; x<bits_in; x++) {
          set_zero *= 2;
        }
        curr_byte -= set_zero; 
        data_bm[full_bytes] = curr_byte;
      }



    //file got bigger
    } else if(old_inode.size < curr_inode.size) {
      int num_blk_to_add = num_blks - old_num_blks;

      //geetthe blocks that direct used to point to, ABSOLUTE BLK IND
      for(int i=0; i<num_blk_to_add; i++) {
        int blk_ind_to_add = (curr_inode.direct[(old_num_blks+i)]) - super_block.data_region_addr;

        int full_bytes  = blk_ind_to_add/8;
        int bits_in = blk_ind_to_add%8;
        int curr_byte = data_bm[full_bytes];

        int set_one = 1;
        for(int x=0; x<bits_in; x++) {
          set_one *= 2;
        }
        curr_byte += set_one; 
        data_bm[full_bytes] = curr_byte;
      }
    }
 
    writeDataBitmap(&super_block, data_bm);

  }

  //put buffer into blocks, write directly into disk
  //put at direct array blk numbeer locatoins
  unsigned char buff[size];
  memcpy(buff, buffer, size);

  // for(int i=0; i<size; i++) {
  //   buff[i] = buffer[i];
  // }

  unsigned char curr_block[(UFS_BLOCK_SIZE)];
  // disk->writeBlock(8, curr_block);

  int direct_ent_ctr = 0;
  //for every byte in the buffer
  for(int byte=0; byte<size; byte++) {
    //if block not filled yet, add to block
    int curr_b_ind = (byte) % UFS_BLOCK_SIZE;
    curr_block[curr_b_ind] = buff[curr_b_ind];
    if((byte == (size-1)) || ((curr_b_ind+1) == UFS_BLOCK_SIZE)) {
      // int curr = curr_block[byte];
      int block_num = curr_inode.direct[direct_ent_ctr];
      direct_ent_ctr += 1;
      // int block_w = block_num + super_block.data_region_addr;
      // curr_block[UFS_BLOCK_SIZE-1] =  '\0'; 
      disk->writeBlock(block_num, curr_block);
    }
    
   
    
  }



  return size;
}







int LocalFileSystem::unlink(int parentInodeNumber, string name) {
  return 0;
}



