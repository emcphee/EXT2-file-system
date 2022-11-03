#include "util.h"

#include "alloc.c"
/************* cd_ls_pwd.c file **************/
int cd()
{

  int ino;
  MINODE* mip;
  u16 i_mode;

  ino = getino(pathname); // finds inode index of path
  mip = iget(running->cwd->dev, ino); // iget {


  // verify mip is a dir
  i_mode = mip->INODE.i_mode;

  if(!S_ISDIR(i_mode)){ // 0100 or 4 is the stated ID for dir in the book
    printf("Error: path is not a dir.\n");
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

  printf("%-10s ", name); // name

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

    ino = getino(pathname);
    if(!ino) return -1; // getino returns 0 on error


    mip = iget(running->cwd->dev, ino); // iget {

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
  MINODE* parent_inode_ptr;
  char my_name[256];
  u16 myino, parentino;

  if(wd == root) return; // base case is root

  // puts i# of . in myino and return i# of ..
  parentino = findino(wd, &myino);

  // get parentino from disk and put it into an minode

  parent_inode_ptr = iget(wd->dev, parentino); // iget {

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
      //enter the new entry as the LAST entry and
      //trim the previous entry rec_len to its ideal_length;
    }
  }
}

void rmkdir(){
  // char *pathname = "doge/cat/real";
  //
  //
  char dirname[64];
  char basename[12];
  char temp[64];
  int i = 0;

  strcpy(temp, pathname);
  char *prev = strtok(temp, "/");
  char *s = prev;
  while(s){
      printf("%s\n", s);
      prev = s;

      s = strtok(0, "/");
      if(s == NULL){
          printf("DIE\n");
          strcpy(basename, prev);
      } else {
          strcat(dirname,  "/");
          strcat(dirname, prev);
      }
      i++;
  }
  printf("DIRNAME - %s\n", dirname);
  printf("BASENAME - %s\n", basename);

  int pino = getino(dirname);
  // if(pino == 0)
  // MINODE* pmip = iget(dev, pino);

  if(pino == 0){
    printf("[!] INO not found\n");
    return;
  } else {
    MINODE* pmip = iget(dev, pino);
    if(search(pmip, basename) == 0){
      printf("[+] Good!\n");
    } else {
      printf("[!] Error dir already exists!\n");
      return;
    }
  }

  // (4).1. Allocate an INODE and a disk block:
  int ino = ialloc(dev);
  int blk = balloc(dev);

  /*
  (4).2. mip = iget(dev, ino) // load INODE into a minode
    initialize mip->INODE as a DIR INODE;
    mip->INODE.i_block[0] = blk; other i_block[ ] = 0;
    mark minode modified (dirty);
    iput(mip); // write INODE back to disk
  */

  MINODE *mip = iget(dev, ino);
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
  /*
    (4).3. make data block 0 of INODE to contain . and .. entries;
  write to disk block blk.
  */
  char buf[BLKSIZE];
  bzero(buf, BLKSIZE);

  DIIR *dp = (DIR*) buf;

  dp->inode = ino;
  dp->rec_len = 12;
  dp->name_len = 1;
  dp->name[0] = '.';
  // // make .. entry: pino=parent DIR ino, blk=allocated block
  dp = (char *)dp + 12;
  dp->inode = pino;
  dp->rec_len = BLKSIZE-12;
  dp->name_len = 12;
  dp->name[0] = d->name[1] = '.';
  put_block(dev, blk, buf);

  /*
  (4).4. enter_child(pmip, ino, basename); which enters
  (ino, basename) as a dir_entry to the parent INODE;
  */

  // if(pmip != NULL){
  //   printf("[+] Success!")
  // } else {
  //   printf("[!] ERROR!\n")
  // }
}
