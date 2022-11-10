/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <ext2fs/ext2_fs.h>

#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

extern MINODE *iget();

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int   n;         // number of component strings

int  fd, dev; // file descriptors

int  nblocks, ninodes, bmap, imap, iblk;

char line[128], cmd[32], pathname[128], pathname2[128]; // input line and its split components


#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "alloc_dealloc.c"
#include "rmdir.c"
#include "link_unlink.c"

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  // initialzes every PROC and MINODE on the stack to defaults values
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i+1;           // pid = 1, 2
    p->uid = p->gid = 0;    // uid = 0: SUPER user
    p->cwd = 0;             // CWD of process
  }
}

// load root INODE and set root pointer to it
int mount_root()
{
  printf("mount_root()\n");
  root = iget(dev, 2);
}

char *disk = "mydisk";     // change this to YOUR virtual

int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  dev = fd;    // sets global for disk (int dev) to opened fd

  /********** read super block  ****************/
  get_block(dev, 1, buf); // block 1 is the super block which contains information about the FS
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){ // checks magic bytes (bytes that identify the file system type)
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf);
  gp = (GD *)buf; // first group descriptor

  bmap = gp->bg_block_bitmap; // pulls integer block location of block_bitmap from GD
  imap = gp->bg_inode_bitmap; // pulls integer block location of inode_bitmap from GD
  iblk = gp->bg_inode_table;  // pulls integer block location of inode_table from GD
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  init();
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  // WRTIE code here to create P1 as a USER process

  while(1){
    printf("input command : [ls|cd|pwd|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;
    pathname2[0] = 0;

    sscanf(line, "%s %s %s", cmd, pathname, pathname2);
    printf("cmd=%s pathname=%s pathname2=%s\n", cmd, pathname, pathname2);

    if (strcmp(cmd, "ls")==0)
       ls();
    else if (strcmp(cmd, "cd")==0)
       cd();
    else if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
    else if(strcmp(cmd, "mkdir") == 0)
      mymkdir();
    else if(strcmp(cmd, "creat") == 0)
      mycreat();
    else if(strcmp(cmd, "rmdir") == 0)
      myrmdir();
    else if(strcmp(cmd, "link") == 0)
      my_link();
    else if(strcmp(cmd, "unlink") == 0)
      my_unlink();
    else if (strcmp(cmd, "quit")==0)
       quit();
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  printf("bye\n");
  exit(0);
}
