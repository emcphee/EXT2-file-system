

// takes in a pathname and two EMPTY STRING pointers dir and base. Tokenizes pathname (nondestructively) and fills the two strings
int pathname_to_dir_and_base(char* pathname, char* dir, char* base){
  char temp[128];
  int i;
  int found = 0;

  strcpy(temp, pathname);

  if(strlen(temp) == 0) return -1; // empty path, dir and base will stay empty
  for(i = strlen(temp) - 1; i >= 1; i--){ // remove trailing '/'s
    if(temp[i] != '/') break;
    temp[i] = 0;
  }

  if(strlen(temp) == 0) return -1; // empty path, dir and base will stay empty

  for(i = strlen(temp) - 1; i >= 1; i--){
    if(temp[i] == '/'){
      found = 1;
      temp[i] = 0;
      break;
    }
  }

  if(found){
    strcpy(base, (temp + i + 1));
    strcpy(dir, temp);
  }else{
    strcpy(base, temp);
  }

  return 0;
}

// takes in a MINODE pointer to parent, the inode number of the child, and the name of the child, and enters it as a DIR entry into the parent
void enter_name(MINODE *pip, int ino, char *name){
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
      pip->INODE.i_block[i] = bno;
      pip->dirty = 1;

      get_block(dev, bno, buf);
      dp = (DIR*) buf;
      cp = buf;

      dp->name_len = strlen(name);
      strncpy(dp->name, name, strlen(name));
      dp->inode = ino;
      dp->rec_len = BLKSIZE;

      put_block(dev, bno, buf);
      return 1;
    }else{
      printf("PANIC! no free blocks available to enter name\n");
      return -1;
    }
}

void rmkdir(){
  char dir[128], base[128], temp[128];
  char *prev, *s; // temps for tokenization
  int pino; // parent inode number
  MINODE* pmip; // parent memory inode pointer
  int ret; // for checking return values
  int i = 0; // tokenization var

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

  // if(pino == 0)
  // MINODE* pmip = iget(dev, pino);

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

  
  int ino = ialloc(dev);
  int blk = balloc(dev);

  MINODE *mip = iget(root->dev, ino);
  INODE *ip = &(mip->INODE);

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
  DIR *dp = (DIR*) buf;
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

  enter_name(pmip, ino, base);

  iput(pmip);
}


// void creat(){
//   // char *pathname = "doge/cat/real";
//   //
//   printf("PATHNAME - %s\n", pathname);
//   //
//   char dirname[64] = {0};
//   char basename[12] = {0};
//   char temp[64] = {0};
//   int i = 0;
//
//   strcpy(temp, pathname);
//   char *prev = strtok(temp, "/");
//   char *s = prev;
//   while(s){
//       printf("%s\n", s);
//       prev = s;
//
//       s = strtok(0, "/");
//       if(s == NULL){
//           printf("DIE\n");
//           strcpy(basename, prev);
//       } else {
//           strcat(dirname,  "/");
//           strcat(dirname, prev);
//       }
//       i++;
//   }
//   printf("DIRNAME - %s\n", dirname);
//   printf("BASENAME - %s\n", basename);
//
//   int pino = getino(dirname);
//   // if(pino == 0)
//   // MINODE* pmip = iget(dev, pino);
//
//   if(pino == 0){
//     printf("[!] INO not found\n");
//     return;
//   } else {
//     MINODE* pmip = iget(dev, pino);
//     if(search(pmip, basename) == 0){
//       printf("[+] Good!\n");
//     } else {
//       printf("[!] Error dir already exists!\n");
//       return;
//     }
//   }
//
//   // (4).1. Allocate an INODE and a disk block:
//   int ino = ialloc(dev);
//   int blk = balloc(dev);
//
//   /*
//   (4).2. mip = iget(dev, ino) // load INODE into a minode
//     initialize mip->INODE as a DIR INODE;
//     mip->INODE.i_block[0] = blk; other i_block[ ] = 0;
//     mark minode modified (dirty);
//     iput(mip); // write INODE back to disk
//   */
//
//   MINODE *mip = iget(dev, ino);
//   INODE *ip = &mip->INODE;
//
//   ip->i_mode = 0x81A4;
//   ip->i_uid = running->uid;
//   ip->i_gid = running->gid;
//   ip->i_size = BLKSIZE;
//   ip->i_links_count = 1;
//   ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
//   ip->i_blocks = 2;
//   ip->i_block[0] = 0;
//   for(int k=1; k<15; k++){
//     ip->i_block[k] = 0;
//   }
//   mip->dirty = 1;
//
//   iput(mip);
//   enter_child(mip, ino, basename);
// }
