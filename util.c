/*********** util.c file ****************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

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
  printf("tokenize %s\n", pathname);

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
       blk    = (ino-1)/8 + iblk; // block number
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

 if (mip==0)
     return;

 mip->refCount--;

 if (mip->refCount > 0) return;
 if (!mip->dirty)       return;


 /* write INODE back to disk */
 block = (mip->ino - 1) / 8 + iblk;
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

   get_block(dev, ip->i_block[0], sbuf);
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
  int i, ino, blk, offset;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;

  // starting mip = root OR CWD
  if (pathname[0]=='/')
     mip = root;
  else
     mip = running->cwd;

  mip->refCount++;         // because we iput(mip) later

  tokenize(pathname);

  for (i=0; i<n; i++){
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);

      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }

      iput(mip);
      mip = iget(dev, ino);
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
   get_block(dev, mip->INODE.i_block[0], buf);
   dp = (DIR *)buf; // dp = first dir ( current dir . )
   cp = buf;
   *myino = dp->inode;
   dp = (DIR*)(cp + dp->rec_len); // dp = second dir ( parent dir .. )
   return dp->inode;

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


// takes in a pathname and two EMPTY STRING pointers dir and base. Tokenizes pathname (nondestructively) and fills the two strings
int pathname_to_dir_and_base(char* pathname, char* dir, char* base){
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

  if(found){
    strcpy(base, (temp + i + 1));
    strcpy(dir, temp);
  }else{
    strcpy(base, temp);
  }

  return 0;
}