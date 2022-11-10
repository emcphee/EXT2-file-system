

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
