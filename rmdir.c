

// unsure if this func works properly but i will assume it does
int check_empty(MINODE *mip){
  char buf[BLKSIZE];
  char *cp;
  DIR *dp;
  int i;
  char dirname[64];

  if(mip->INODE.i_links_count > 2){
    return 0;
  } else if(mip->INODE.i_links_count == 2){
    for(i=0; i<12; i++){
      if(mip->INODE.i_block[i] == 0){
        break;
      }

      get_block(mip->dev, mip->INODE.i_block[i], buf);
      dp = (DIR *) buf;
      cp = buf;

      while(cp < buf + BLKSIZE){
        strncpy(dirname, dp->name, dp->name_len);
        dirname[dp->name_len] = 0;
        if(strcmp(dirname, ".") && strcmp(dirname, "..")){
          return 0;
        }
        cp += dp->rec_len;
        dp = (DIR *)cp;
      }
    }
  }
  return 1;
}


void rmchild(MINODE* pmip, char dir_name[]){
  int i,j;
  int blk_to_remove;
  char *cp, temp[256], sbuf[BLKSIZE];
  int len_to_cpy;
  DIR *dp, *pdp, *dp_to_remove, *dp_before_remove;
  int found = 0;
  for(i = 0; i < 12; i++){ // i from 0 to 11 because we are only doing direct blocks
    get_block(pmip->dev, pmip->INODE.i_block[i], sbuf);
    pdp = 0;
    dp = (DIR*)sbuf;
    cp = sbuf; // byte pointer

    while(cp < sbuf + BLKSIZE){ // while cp is not outside block starting at sbuf
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      if(!strcmp(temp, dir_name)){
        dp_to_remove = dp;
        dp_before_remove = pdp;
        found = 1;
      }
      pdp = dp;
      cp += dp->rec_len; // increment rec_len number of bytes
      dp = (DIR*)cp; // set dp to point to next dir
    }
    if(found) break;
  }

  if(dp_to_remove->rec_len == BLKSIZE){ // this dir entry is the only one in the block, so we deallocate the block
    if(i < 11 && pmip->INODE.i_block[i + 1]){ // there is another block after the one we are removing
      for(j = i + 1; j < 12 && pmip->INODE.i_block[j]; j++); // ASSUMING ONLY FIRST 12 BLOCKS HERE
      blk_to_remove = pmip->INODE.i_block[i];
      pmip->INODE.i_block[i] = pmip->INODE.i_block[j - 1]; // fill in empty space left by removed block in i_block[]
      pmip->INODE.i_block[j - 1] = 0;
    }else{ // there is no other blocks
      blk_to_remove = pmip->INODE.i_block[i];
      pmip->INODE.i_block[i] = 0;
    }
    bdalloc(pmip->dev, blk_to_remove);
    pmip->INODE.i_size -= BLKSIZE;
    put_block(dev, pmip->INODE.i_block[i], sbuf);
    return;
  }
  if((char*)dp_to_remove + dp_to_remove->rec_len == sbuf + BLKSIZE){ // this dir entry is the last entry in the block, just remove it and set rec_len of previous to fill the block
    dp_before_remove->rec_len += dp_to_remove->rec_len;
    put_block(dev, pmip->INODE.i_block[i], sbuf);
    return;
  }
// last option is that the dir entry is on a block with more entries after it, so the entries have to be pushed back when its removed.
  pdp->rec_len += dp_to_remove->rec_len;
  memcpy((char*)dp_to_remove, (char*) dp_to_remove + dp_to_remove->rec_len, sbuf - (char*)dp_to_remove + BLKSIZE);
  put_block(dev, pmip->INODE.i_block[i], sbuf);
}


void myrmdir(){
  int ino, pino, i;
  MINODE* mip, *pmip;
  char dir_name[128];
  ino = getino(pathname);

  if(ino == 2){
    printf("Error: Cannot remove root.\n");
    return;
  }
  mip = iget(dev, ino);
  if(!S_ISDIR(mip->INODE.i_mode)){
    printf("Error: Not a directory.\n");
    iput(mip);
    return;
  }
  if(mip->refCount > 1){
    printf("Error: Directory is busy.\n");
    iput(mip);
    return;
  }
  if(!check_empty(mip)){
    printf("Error: Directory is not empty.\n");
    iput(mip);
    return;
  }

  pino = findino(mip, &ino);
  pmip = iget(mip->dev, pino);

  findmyname(pmip, ino, dir_name);
  // works perfect until here... maybe a problem in rmchild()
  rmchild(pmip, dir_name);
  pmip->INODE.i_links_count--;
  pmip->dirty = 1;

  iput(pmip); // im thinking iput is not actually putting for some reason :/
  
  bdalloc(mip->dev, mip->INODE.i_block[0]);
  idalloc(mip->dev, mip->ino);
  iput(mip);
}
