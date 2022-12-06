/************* cd_ls_pwd.c file **************/
int cd()
{

  int ino;
  MINODE* mip;
  u16 i_mode;

  if(!my_access(pathname, EXECUTEPERM|READPERM)){
    printf("Error: invalid permissions.\n");
    return -1;
  }

  ino = getino(pathname); // finds inode index of path
  mip = iget(dev, ino); // iget {

  printf("ino = %d, mip = %x\n", ino, mip);
  // verify mip is a dir
  i_mode = mip->INODE.i_mode;

  if(!S_ISDIR(i_mode)){ 
    printf("Error: path is not a dir.\n");
    printf("mode = %x\n", i_mode);
    iput(mip);
    return -1;
  }
  iput(running->cwd); // } iput
  running->cwd = mip; // set new cwd to mip

  return 0; // 0 for success (?)

  // READ Chapter 11.7.3 HOW TO chdir

}

// modified from ls -l function from book not sure if its all correct
// because the books 'mode' variable is from stat but this is from inode
int ls_file(MINODE *mip, char *name)
{

  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";
  char t[2] = "\0\0";
  char ftime[128];
  int i;
  u16 i_mode;
  time_t time;
  char temp[128];

  i_mode = mip->INODE.i_mode;

  if(S_ISREG(i_mode)) printf("%c", '-');
  if(S_ISDIR(i_mode)) printf("%c", 'd');
  if(S_ISLNK(i_mode)) printf("%c", 'l');


  for(i = 8; i >= 0; i--){ // permissions
    if(i_mode & (1 << i)){
      printf("%c", t1[i]);
    }
    else{
      printf("%c", t2[i]);
    }
  }

  printf("%4d ", mip->INODE.i_links_count); // link count
  printf("%4d ", mip->INODE.i_gid); // gid
  printf("%4d ", mip->INODE.i_uid); // uid
  printf("%8d ", mip->INODE.i_size); // filesize

  time = mip->INODE.i_ctime;
  strcpy(ftime, ctime(&time));
  ftime[strlen(ftime) - 1] = 0; // remove newline
  printf("%s ", ftime); // ctime

  if (S_ISLNK(i_mode)){
    strcpy(temp, name);
    strcat(temp, " -> ");
    strcat(temp, (char*)mip->INODE.i_block);
    printf("%-15s ", temp); // name with link
  }else{
    printf("%-15s ", name); // name
  }

  printf("[%d %d]", mip->dev, mip->ino); // [dev, ino]

  printf("\n");

  // READ Chapter 11.7.3 HOW TO ls
}

int ls_dir(MINODE *mip)
{

  int i;
  char sbuf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  MINODE* entry;
  for(i = 0; i < 12; i++){ // i from 0 to 11 because we are only doing direct blocks
    if(mip->INODE.i_block[i] == 0) {
      return 0; // i_block is null terminated
    }
    get_block(mip->dev, mip->INODE.i_block[i], sbuf); // get next block to sbuf
    dp = (DIR*)sbuf;
    cp = sbuf; // byte pointer
    while(cp < sbuf + BLKSIZE){ // while cp is not outside block starting at sbuf
      strncpy(temp, dp->name, dp->name_len); // not null terminated
      temp[dp->name_len] = 0; // add null terminator

      entry = iget(mip->dev, dp->inode); // iget {

      if(!entry) return -1; // iget returns 0 on error
      ls_file(entry, temp);

      iput(entry); // } iput
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }

  }

  printf("\n");
  return 0;
}

int ls()
{
  int ino, r;
  MINODE* mip;
  if(!pathname[0]){ // if pathname[0] is null, then ls is the only arg
    r = ls_dir(running->cwd); // ls_dir on cwd

  }else{ // pathname[0] is not null so theres a second arg for path
    if(!my_access(pathname, EXECUTEPERM|READPERM)){
    printf("Error: invalid permissions.\n");
    return -1;
  }
    ino = getino(pathname);
    if(!ino) return -1; // getino returns 0 on error


    mip = iget(dev, ino); // iget {

    if(!S_ISDIR((mip->INODE.i_mode))){
      printf("Error: Cannot ls non-dir.\n");
      iput(mip);
      return -1;
    }

    if(!mip) return -1; // iget returns 0 on error

    r = ls_dir(mip);

    iput(mip); // } iput
  }
  return r;
}

char *pwd(MINODE *wd)
{
  if (wd == root){
    printf("/\n");
    return 0;
  }else{
    // not root, call rpwd
    rpwd(wd);
    printf("\n");
    return 0;
  }

}

void rpwd(MINODE *wd){
  int i;
  MINODE* parent_inode_ptr;
  char my_name[256];
  u16 myino, parentino;
  MOUNT* pmnt;
  MINODE* tmip;
  
  if(wd == root) return; // base case is root

  // puts i# of . in myino and return i# of ..
  parentino = findino(wd, &myino);
  if(parentino == 2 && wd->ino == 2 && wd->dev != mountTable[0].dev){
    for(i = 0; i < 8; i++){
      if(mountTable[i].dev == wd->dev) break;
    }
    parentino = findino(mountTable[i].mounted_inode, &myino);
    pmnt = &mountTable[i];
    tmip = pmnt->mounted_inode;
    parent_inode_ptr = iget(tmip->dev, parentino);
  }else{
    parent_inode_ptr = iget(wd->dev, parentino); 
  }
  printf("parentino = %d\n", parentino);

  if(!parent_inode_ptr) return; // iget returns 0 on error

  // find wd's name in parent from its inode
  if(findmyname(parent_inode_ptr, myino, my_name)){
    printf("Error: findmyname could not find child with myino.\n");
    return;
  }

  // recursive call before print for reverse order
  rpwd(parent_inode_ptr);

  iput(parent_inode_ptr); // } iput

  printf("/%s", my_name);
  return;
}
