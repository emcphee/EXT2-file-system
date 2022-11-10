
// takes in a MINODE pointer to parent, the inode number of the child, and the name of the child, and enters it as a DIR entry into the parent
int enter_name(MINODE *pip, int ino, char *name){
  char buf[BLKSIZE];
  int i, need_len, ideal_len, remaining, bno, free_block;
  DIR *dp;
  char *cp;

  free_block = 0;

  for(i=0; i<12; i++){
    if(pip->INODE.i_block[i] == 0){
      free_block = i;
      break;
    }

    get_block(pip->dev, pip->INODE.i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    need_len = 4*((8 + strlen(name) + 3) /4); // record length of new entry


    while (cp + dp->rec_len < buf + BLKSIZE){
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }

    ideal_len = 4*((8 + dp->name_len + 3) /4); // ideal length of dp

    remaining = dp->rec_len - ideal_len;
    if(remaining >= need_len){ // we can insert the DIR entry after the last entry
      dp->rec_len = ideal_len;

      cp += dp->rec_len;
      dp = (DIR *)cp;

      dp->inode = ino;
      strncpy(dp->name, name, strlen(name));
      dp->name_len = strlen(name);
      dp->rec_len = remaining;

      put_block(dev, pip->INODE.i_block[i], buf);
      return 0;
      //enter the new entry as the LAST entry and
      //trim the previous entry rec_len to its ideal_length;
    }
  }
  if(free_block != 0){
      pip->INODE.i_size += BLKSIZE;
      bno = balloc(dev);
      pip->INODE.i_block[free_block] = bno;
      pip->dirty = 1;

      get_block(dev, bno, buf);
      dp = (DIR*) buf;
      cp = buf;

      dp->name_len = strlen(name);
      strncpy(dp->name, name, strlen(name));
      dp->inode = ino;
      dp->rec_len = BLKSIZE;

      put_block(dev, bno, buf);
      return 0;
    }else{
      printf("PANIC! no free blocks available to enter name\n");
      return -1;
    }
}

void mymkdir(){
  char dir[128], base[128], temp[128];
  int pino, ino, blk;
  MINODE *pmip, *mip;
  INODE *ip;
  DIR *dp;
  int ret;
  int i = 0;

  // initialize both to empty strings
  dir[0] = 0;
  base[0] = 0;


  // input of pathname to create dir, consisting of dirname and basename
  printf("PATHNAME - %s\n", pathname);

  ret = pathname_to_dir_and_base(pathname, dir, base); // breaks pathname into dir and base
  if(ret){
    printf("Error: invalid pathname\n");
    return;
  }
  // dirname and basename should not be separated so we print them to verify
  printf("DIRNAME - %s\n", dir);
  printf("BASENAME - %s\n", base);

  //
  if(strlen(dir) == 0){
    pino = running->cwd->ino;
  }else{
    pino = getino(dir);
  }

  if(pino == 0){
    printf("[!] INO not found\n");
    return;
  } else {
    pmip = iget(dev, pino);
    if(search(pmip, base) == 0){
      printf("[+] Good! file doesn't already exist.\n");
    } else {
      printf("[!] Error dir already exists!\n");
      return;
    }
  }

  
  ino = ialloc(dev);
  blk = balloc(dev);

  mip = iget(root->dev, ino);
  ip = &(mip->INODE);

  ip->i_mode = 0x41ED;
  ip->i_uid = running->uid;
  ip->i_gid = running->gid;
  ip->i_size = BLKSIZE;
  ip->i_links_count = 2;
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
  ip->i_blocks = 2;
  ip->i_block[0] = blk;
  for(int k=1; k<15; k++){
    ip->i_block[k] = 0;
  }
  mip->dirty = 1;
  iput(mip);
  // /*
  //   (4).3. make data block 0 of INODE to contain . and .. entries;
  // write to disk block blk.
  // */
  char buf[BLKSIZE];
  bzero(buf, BLKSIZE);
  get_block(dev, blk, buf);
  dp = (DIR*) buf;
  dp->inode = ino;
  dp->rec_len = 12;
  dp->name_len = 1;
  dp->name[0] = '.';
  // // make .. entry: pino=parent DIR ino, blk=allocated block
  dp = (char *)dp + 12;
  dp->inode = pino;
  dp->rec_len = BLKSIZE-12;
  dp->name_len = 2;
  dp->name[0] = dp->name[1] = '.';
  put_block(dev, blk, buf);

  ret = enter_name(pmip, ino, base);
  if(ret){
    printf("Error: something went very wrong in mkdir. Could not add new dir name to parent.\n");
    return;
  }
  pmip->INODE.i_links_count++; // increase links for a new child dir
  iput(pmip);
}


void mycreat(){
  char dir[128], base[128], temp[128];
  int pino, ino;
  MINODE *pmip, *mip;
  INODE *ip;
  int ret;
  int i = 0;


  //input of pathname to create dir, consisting of dirname and basename
  printf("PATHNAME - %s\n", pathname);

  ret = pathname_to_dir_and_base(pathname, dir, base); // breaks pathname into dir and base
  if(ret){
    printf("Error: invalid pathname\n");
    return;
  }
  //dirname and basename should not be separated so we print them to verify
  printf("DIRNAME - %s\n", dir);
  printf("BASENAME - %s\n", base);

  
  if(strlen(dir) == 0){
    pino = running->cwd->ino;
  }else{
    pino = getino(dir);
  }

  if(pino == 0){
    printf("[!] INO not found\n");
    return;
  } else {
    pmip = iget(dev, pino);
    if(search(pmip, base) == 0){
      printf("[+] Good! file doesn't already exist.\n");
    } else {
      printf("[!] Error dir already exists!\n");
      return;
    }
  }

  
  ino = ialloc(dev);
  if(!ino){
    printf("Error: ino couldn't be allocated.\n");
    iput(pmip);
    return 0;
  }

  mip = iget(root->dev, ino);
  ip = &(mip->INODE);

  ip->i_mode = 0x81A4; // 0644
  ip->i_uid = running->uid;
  ip->i_gid = running->gid;
  ip->i_size = 0;
  ip->i_links_count = 1;
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
  ip->i_blocks = 0;
  ip->i_block[0] = 0;
  for(int k=1; k<15; k++){
    ip->i_block[k] = 0;
  }
  mip->dirty = 1;
  iput(mip);
  

  ret = enter_name(pmip, ino, base);
  if(ret){
    printf("Error: something went very wrong in creat. Could not add new dir name to parent.\n");
    return;
  }

  iput(pmip);
}
