


void enter_name(MINODE *pip, int ino, char *name){
  char buf[BLKSIZE];

  for(int i=0; i<12; i++){
    if(pip->INODE.i_block[i] == 0){
      break;
    }

    get_block(pip->dev, pip->INODE.i_block[i], buf);
    DIR *dp = (DIR *)buf;
    char *cp = buf;

    int ideal_len = 4*((8 + dp->name_len + 3) /4);
    int need_len = 4*((8 + strlen(name) + 3) /4);


    while (cp + dp->rec_len < buf + BLKSIZE){
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }

    int remaining = dp->rec_len - ideal_len;
    if(remaining >= need_len){
      dp->rec_len = ideal_len;

      cp += dp->rec_len;
      dp = (DIR *)cp;

      dp->inode = ino;
      strcpy(dp->name, name);
      dp->name_len = strlen(name);
      dp->rec_len = remaining;

      put_block(dev, pip->INODE.i_block[i], buf);
      return 0;
      //enter the new entry as the LAST entry and
      //trim the previous entry rec_len to its ideal_length;
    } else {
      pip->INODE.i_size = BLKSIZE;
      int bno = balloc(dev);
      pip->INODE.i_block[i] = bno;
      pip->dirty = 1;

      get_block(dev, bno, buf);
      dp = (DIR*) buf;
      cp = buf;

      dp->name_len = strlen(name);
      strcpy(dp->name, name);
      dp->inode = ino;
      dp->rec_len = BLKSIZE;

      put_block(dev, bno, buf);
      return 1;
    }
  }
}

void rmkdir(){
  // char *pathname = "doge/cat/real";
  //
  printf("PATHNAME - %s\n", pathname);
  //
  char dirname[128];
  char basename[128];
  char temp[128];
  int i = 0;

  dirname[0] = 0;
  basename[0] = 0;

  MINODE* pmip;

  strcpy(temp, pathname);
  char *prev = strtok(temp, "/");
  char *s = prev;
  // check for special case where / is at start of path meaning to start from root
  if(s && s[0] == 0){ // if delim is / and first char is / s will equal empty string
    printf("starting from root\n");
    s = strtok(temp, '/');
    strcat(dirname, "/");
    i++; // not sure what i is for but I increment it here (?)
  }
  while(s){
      printf("%s\n", s);
      prev = s;

      s = strtok(0, "/");
      if(s == NULL){
          strcpy(basename, prev);
      } else {
          // this if statement is super bodgy but it works
          if( !(i == 1 && dirname[0] == '/') ) strcat(dirname,  "/");
          strcat(dirname, prev);
      }
      i++;
  }
  printf("DIRNAME - %s\n", dirname);
  printf("BASENAME - %s\n", basename);

  int pino;
  if(strlen(dirname) == 0){
    pino = running->cwd->ino;
  }else{
    pino = getino(dirname);
  }

  // if(pino == 0)
  // MINODE* pmip = iget(dev, pino);

  if(pino == 0){
    printf("[!] INO not found\n");
    return;
  } else {
    pmip = iget(dev, pino);
    if(search(pmip, basename) == 0){
      printf("[+] Good!\n");
    } else {
      printf("[!] Error dir already exists!\n");
      return;
    }
  }

  // // (4).1. Allocate an INODE and a disk block:
  int ino = ialloc(dev);
  int blk = balloc(dev);
  //
  // /*
  // (4).2. mip = iget(dev, ino) // load INODE into a minode
  //   initialize mip->INODE as a DIR INODE;
    // mip->INODE.i_block[0] = blk; other i_block[ ] = 0;
    // mark minode modified (dirty);
    // iput(pmip); // write INODE back to disk
  // */
  //
  MINODE *mip = iget(root->dev, ino);
  INODE *ip = &mip->INODE;

  ip->i_mode = 0x41ED;
  ip->i_uid = running->uid;
  ip->i_gid = running->gid;
  ip->i_size = BLKSIZE;
  ip->i_links_count = 2;
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
  ip->i_blocks = 2;
  ip->i_block[0] = blk;
  for(int k=1; k<14; k++){
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
  dp->name_len = 12;
  dp->name[0] = dp->name[1] = '.';
  put_block(dev, blk, buf);

  // /*
  // (4).4. enter_child(pmip, ino, basename); which enters
  // (ino, basename) as a dir_entry to the parent INODE;
  // */
  enter_name(mip, ino, basename);
  //
  // // if(pmip != NULL){
  // //   printf("[+] Success!")
  // // } else {
  // //   printf("[!] ERROR!\n")
  // // }
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
