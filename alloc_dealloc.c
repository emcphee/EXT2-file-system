
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


int incFreeInodes(int dev)
{
  char buf[BLKSIZE];

  // inc free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
  return 0;
}

int incFreeBlocks(int dev)
{
  char buf[BLKSIZE];

  // inc free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buf);
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
  GD* gpL;
  SUPER* spL;
  int imapL, ninodesL;
  get_block(dev, 1, buf);
  spL = (SUPER *)buf;
  ninodesL = spL->s_inodes_count;

  get_block(dev, 2, buf);
  gpL = (GD *)buf;
  imapL = gpL->bg_inode_bitmap;


  // read inode_bitmap block
  get_block(dev, imapL, buf);

  for (i=0; i < ninodesL; i++){ // use ninodes from SUPER block
    if (tst_bit(buf, i)==0){
      set_bit(buf, i);
      put_block(dev, imapL, buf);

      decFreeInodes(dev);

      //printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
      return i+1;
    }
  }
  return 0;
}

int balloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  GD* gpL;
  SUPER* spL;
  int nblocksL, bmapL;

  get_block(dev, 1, buf);
  spL = (SUPER *)buf;
  nblocksL = spL->s_blocks_count;

  get_block(dev, 2, buf);
  gpL = (GD *)buf;
  bmapL = gpL->bg_block_bitmap;


  get_block(dev, bmapL, buf);

  for (int i = 0; i < nblocksL; i++) {
    if (tst_bit(buf, i) == 0) {
        set_bit(buf, i);
        decFreeBlocks(dev);

        put_block(dev, bmapL, buf);
        return i+1;
    }
}

 
  printf("Error: No more blocks to allocate.\n");
  return 0;
}

int idalloc(int dev, int ino)
{
  int i;
  char buf[BLKSIZE];

  GD* gpL;
  SUPER* spL;
  int imapL, ninodesL;
  get_block(dev, 1, buf);
  spL = (SUPER *)buf;
  ninodesL = spL->s_inodes_count;

  get_block(dev, 2, buf);
  gpL = (GD *)buf;
  imapL = gpL->bg_inode_bitmap;

  if (ino > ninodesL){
    printf("Error: inumber %d out of range\n", ino);
    return -1;
  }

  // get inode bitmap block
  get_block(dev, imapL, buf);
  clr_bit(buf, ino-1);

  // write buf back
  put_block(dev, imapL, buf);

  // update free inode count in SUPER and GD
  incFreeInodes(dev);
  return 0;
}

int bdalloc(int dev, int blk)
{
  int i;
  char buf[BLKSIZE];

  GD* gpL;
  SUPER* spL;
  int nblocksL, bmapL;

  get_block(dev, 1, buf);
  spL = (SUPER *)buf;
  nblocksL = spL->s_blocks_count;

  get_block(dev, 2, buf);
  gpL = (GD *)buf;
  bmapL = gpL->bg_block_bitmap;

  if (blk > nblocksL){
    printf("Error: blocknumber %d out of range\n", blk);
    return -1;
  }

  // get inode bitmap block
  get_block(dev, bmapL, buf);
  clr_bit(buf, blk-1);

  // write buf back
  put_block(dev, bmapL, buf);

  // update free inode count in SUPER and GD
  incFreeBlocks(dev);
  return 0;
}
