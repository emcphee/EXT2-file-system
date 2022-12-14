/*********** util.c file ****************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>


/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}

int tokenize(char *pathname)
{
  int i;
  char *s;
  //printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }
  name[n] = 0;

  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;
  GD* gpL;
  int iblkL;

  get_block(dev, 2, buf);
  gpL = (GD *)buf;
  iblkL = gpL->bg_inode_table;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    // if it has at least one reference, the origin disk is the same, and it has the same inode index
    if (mip->refCount && mip->dev == dev && mip->ino == ino){
       mip->refCount++; // add another reference
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino into buf[ ]
       blk    = (ino-1)/8 + iblkL; // block number
       offset = (ino-1) % 8; // block offset

       //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);    // buf[ ] contains this INODE
       ip = (INODE *)buf + offset;  // this INODE in buf[ ]
       // copy INODE to mp->INODE
       mip->INODE = *ip;
       return mip;
    }
  }
  printf("PANIC: no more free minodes\n");
  return 0;
}

void iput(MINODE *mip)  // iput(): release a minode
{
  int i, block, offset;
  char buf[BLKSIZE];
  INODE *ip;

  GD* gpL;
  int iblkL;

  get_block(dev, 2, buf);
  gpL = (GD *)buf;
  iblkL = gpL->bg_inode_table;

  if (mip==0)
      return;

  mip->refCount--;

  if (mip->refCount > 0) return;
  if (!mip->dirty)       return;


  /* write INODE back to disk */
  block = (mip->ino - 1) / 8 + iblkL;
  offset = (mip->ino - 1) % 8;

  get_block(mip->dev, block, buf);
  ip = (INODE *)buf + offset;
  *ip = mip->INODE;
  put_block(mip->dev, block, buf);

  midalloc(mip);

}

// this seems unfinished but book is not clear
int midalloc(MINODE* mip){
  if(!mip){
    printf("Error: midalloc() passed a null pointer.\n");
    return -1;
  }
  mip->refCount = 0;
  return 0;
}

int search(MINODE *mip, char *name)
{
   int i;
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;


   printf("search for %s in MINODE = [%d, %d]\n", name,mip->dev,mip->ino);
   ip = &(mip->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(mip->dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   printf("  ino   rlen  nlen  name\n");

   while (cp < sbuf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len); // dp->name is NOT a string
     temp[dp->name_len] = 0;                // temp is a STRING
     printf("%4d  %4d  %4d    %s\n",
	    dp->inode, dp->rec_len, dp->name_len, temp); // print temp !!!

     if (strcmp(temp, name)==0){            // compare name with temp !!!
        printf("found %s : ino = %d\n", temp, dp->inode);
        return dp->inode;
     }

     cp += dp->rec_len;
     dp = (DIR *)cp;
   }
   return 0;
}

int getino(char *pathname) // return ino of pathname
{
  int i, j, ino, blk, offset;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;
  MOUNT *mnt;

  if(!my_access(pathname, READPERM|WRITEPERM)){
    printf("Error: invalid permissions.\n");
    return 0;
  }

  if(!pathname[0]) return running->cwd->ino;

  //printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0){
    dev = mountTable[0].dev;
    return 2;
  }

  // starting mip = root OR CWD
  if (pathname[0]=='/')
     mip = root;
  else
     mip = running->cwd;

  mip->refCount++;         // because we iput(mip) later

  tokenize(pathname);

  for (i=0; i<n; i++){
      //printf("===========================================\n");
      //printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);

      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }

      if(ino == 2 && mip->ino == 2 && mip->dev != mountTable[0].dev){ // upward traversal
        for(j = 0; j < 8; j++){
          if(mip->dev == mountTable[j].dev) break;
        }
        if(j > 7){
          printf("Error: Mount not found in getino, big error.\n");
          exit(1);
        }
        iput(mip);
        mip = mountTable[j].mounted_inode;
        ino = search(mip, ".."); // find parent of mounted inode
        mip = iget(mip->dev, ino);
        dev = mip->dev;
        
      }else{ // normal cd or downward traversal
        iput(mip);
        mip = iget(mip->dev, ino);
        if(mip->mounted){ // downward traversal
            mnt = mip->mptr;
            iput(mip);
            mip = iget(mnt->dev, 2);
            dev = mip->dev;
            ino = mip->ino;
        }
      }
   }

   iput(mip);
   return ino;
}

// These 2 functions are needed for pwd()
int findmyname(MINODE *parent, u32 myino, char myname[ ])
{
  int i;
  char *cp, temp[256], sbuf[BLKSIZE];
  DIR *dp;
  for(i = 0; i < 12; i++){ // i from 0 to 11 because we are only doing direct blocks

    if(parent->INODE.i_block[i] == 0) return -1; // i_block is null terminated

    get_block(parent->dev, parent->INODE.i_block[i], sbuf);
    dp = (DIR*)sbuf;
    cp = sbuf; // byte pointer

    while(cp < sbuf + BLKSIZE){ // while cp is not outside block starting at sbuf

      if(dp->inode == myino){
        strncpy(temp, dp->name, dp->name_len); // name is not null terminated
        temp[dp->name_len] = 0; // add null terminator
        strcpy(myname, temp);
        return 0;
      }

      cp += dp->rec_len; // increment rec_len number of bytes
      dp = (DIR*)cp; // set dp to point to next dir
    }

  }
  return -1; // didn't find matching name in for loop, doesn't exist. return null
}

int findino(MINODE *mip, u32 *myino) // myino = i# of . return i# of ..
{
  // mip points at a DIR minode
  // WRITE your code here: myino = ino of .  return ino of ..
  // all in i_block[0] of this DIR INODE.

   char buf[BLKSIZE];
   DIR *dp;
   char *cp;

      // gets first i_block
   get_block(mip->dev, mip->INODE.i_block[0], buf);
   dp = (DIR *)buf; // dp = first dir ( current dir . )
   cp = buf;
   *myino = dp->inode;
   dp = (DIR*)(cp + dp->rec_len); // dp = second dir ( parent dir .. )
   return dp->inode;

}

// takes in a pathname and two EMPTY STRING pointers dir and base. Tokenizes pathname (nondestructively) and fills the two strings
int pathname_to_dir_and_base(char* pathname, char* dir, char* base){
  int rt;
  char temp[128];
  int i, found;
  found = 0;
  strcpy(temp, pathname);
  dir[0] = 0;
  base[0] = 0;

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
  rt = 0;
  if(temp[0] == '/')rt = 1;

  if(found){
    strcpy(base, (temp + i + 1));
    if(rt){
      strcpy(dir, "/");
      strcat(dir, temp);
    }else{
      strcpy(dir, temp);
    }
  }else{
    if(rt){
      strcpy(dir, "/");
      strcpy(base, temp + 1);
    }else{
      strcpy(base, temp);
    }
    
  }

  return 0;
}

/*
OWNER
R- 0x0100
W- 0x0080
X- 0x0040

OTHER
R- 0x0004
W- 0x0002
X- 0x0001
*/

int my_access(char* filename, int mode){

  int r;
  int ino;
  MINODE* mip;
  int fileMode;
  if(running->uid == 0){
    printf("SUPERUSER - ACCESS GRANTED uid = %d\n", running->uid);
    return 1;
  }
  r = 1;
  mip = iget(dev, ino);
  fileMode = mip->INODE.i_mode;
  if(mip->INODE.i_uid == running->uid){ // OWNER
    if( (mode & READPERM) && !(fileMode & 0x0100) ) r = 0;
    if( (mode & WRITEPERM) && !(fileMode & 0x0080) ) r = 0;
    if( (mode & EXECUTEPERM) && !(fileMode & 0x0040) ) r = 0;
  }else{ // OTHER
    if(mode & ISOWNER) r = 0;
    if( (mode & READPERM) && !(fileMode & 0x0004) ) r = 0;
    if( (mode & WRITEPERM) && !(fileMode & 0x0002) ) r = 0;
    if( (mode & EXECUTEPERM) && !(fileMode & 0x0001) ) r = 0;
  }
  return r;
  
  return 1;
}
