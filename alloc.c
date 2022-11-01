#include "type.h"

extern int imap;
extern int ninodes;
extern int bmap;
extern int nblocks;

int tst_bit(char *buf, int bit){
    int block, offset;
    block = bit / 8;
    offset = bit % 8;
    return buf[block] & (1 << offset);
}

int set_bit(char *buf, int bit){
    int block, offset;
    block = bit / 8;
    offset = bit % 8;
    buf[block] |= (1 << offset);
    return 0;
}

int clr_bit(char *buf, int bit){
    int block, offset;
    block = bit / 8;
    offset = bit % 8;
    buf[block] &= !(1 << offset);
    return 0;
}

int decFreeInodes(int dev)
{
  char buf[BLKSIZE];
  SUPER* sp;
  GD* gp;
  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);

  return 0;
}

int decFreeBlocks(int dev)
{
  char buf[BLKSIZE];
  SUPER* sp;
  GD* gp;
  // dec free blocks count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);

  return 0;
}

int ialloc(int dev)  // allocate an inode number from inode_bitmap
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){ // use ninodes from SUPER block
    if (tst_bit(buf, i)==0){
        set_bit(buf, i);
	put_block(dev, imap, buf);

	decFreeInodes(dev);

	printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
        return i+1;
    }
  }
  return 0;
}

int balloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  get_block(dev, bmap, buf);

  for (i=0; i < nblocks; i++){
    if (tst_bit(buf, i)==0){
        set_bit(buf, i);
	put_block(dev, bmap, buf);

	decFreeBlocks(dev);

	printf("allocated block num = %d\n", i); // i think blocks count from 0 ? if not change to +1
        return i;
    }
  }
  return 0;
}
