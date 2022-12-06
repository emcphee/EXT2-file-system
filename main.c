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
MOUNT mountTable[8];

OFT oft[64]; // OFTs

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int   n;         // number of component strings

int  fd, dev; // file descriptors

int  nblocks, ninodes, bmap, imap, iblk;

char line[128], cmd[32], pathname[128], pathname2[128]; // input line and its split components

// for permission checking
#define READPERM 1
#define WRITEPERM 2
#define EXECUTEPERM 4
#define ISOWNER 8

#define DEFAULT_UID 0


#include "util.c"
#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "alloc_dealloc.c"
#include "rmdir.c"
#include "link_unlink.c"
#include "symlink.c"

#include "mount_umount.c"

#include "open_close.c"
#include "read_cat.c"
#include "write_cp.c"

#include "debug_functions.c"

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  //printf("init()\n");

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
    p->uid = p->gid = DEFAULT_UID;    // uid = 0: SUPER user
    p->cwd = 0;             // CWD of process
    for(j = 0; j < NFD; j++){
      p->fd[j] = 0;
    }
  }
  for(i = 0; i < 64; i++){
    oft[i].refCount = 0;
  }
  for(i = 0; i < 8; i++){
    mountTable[i].dev = 0;
  }
}

// load root INODE and set root pointer to it
int mount_root()
{
  MOUNT *m;
  //printf("mount_root()\n");
  root = iget(dev, 2);

  m = &mountTable[0];
  m->dev = dev;
  m->ninodes = ninodes;
  m->nblocks = nblocks;
  m->bmap = bmap;
  m->imap = imap;
  m->iblk = iblk;

  return 0;
}

int cmd_index(char* command){
  int i;            // 0     1     2        3      4        5        6          7        8         9     10     11    12        13      14   15
  char* commands[] = {"ls", "cd", "pwd", "mkdir", "creat", "rmdir", "link", "unlink", "symlink", "cat", "cp", "quit", "mv", "mount", "umount", 0};
  for(i = 0; commands[i]; i++){
    if(!strcmp(command, commands[i])) break;
  }
  return i;
}


char *disk = "disk2";     // change this to YOUR virtual

int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];

  //printf("checking EXT2 FS ....");
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
  //printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf);
  gp = (GD *)buf; // first group descriptor

  bmap = gp->bg_block_bitmap; // pulls integer block location of block_bitmap from GD
  imap = gp->bg_inode_bitmap; // pulls integer block location of inode_bitmap from GD
  iblk = gp->bg_inode_table;  // pulls integer block location of inode_table from GD
  //printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  init();
  mount_root();
  //printf("root refCount = %d\n", root->refCount);

  //printf("creating P0 as running process\n");
  running = &proc[1];
  running->cwd = iget(dev, 2);
  //printf("root refCount = %d\n", root->refCount);

  // WRTIE code here to create P1 as a USER process

  while(1){

    printf(">> ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (!line[0]) continue;

    pathname[0] = pathname2[0] = 0;

    sscanf(line, "%s %s %s", cmd, pathname, pathname2);
    //printf("cmd=%s pathname=%s pathname2=%s\n", cmd, pathname, pathname2);

    switch(cmd_index(cmd)){
      case 0: ls(); break;
      case 1: cd(); break;
      case 2: pwd(running->cwd); break;
      case 3: mymkdir(); break;
      case 4: mycreat(pathname); break;
      case 5: myrmdir(); break;
      case 6: my_link(pathname, pathname2); break;
      case 7: my_unlink(pathname); break;
      case 8: my_symlink(); break;
      case 9: my_cat(pathname); break;
      case 10: my_cp(pathname, pathname2); break;
      case 11: quit(); break;
      case 12: my_mv(pathname, pathname2); break;
      case 13: mount(pathname, pathname2); break;
      case 14: umount(pathname);
      default: break;
    }
    
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i = 0; i < NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0) iput(mip);
  }
  printf("exit success\n");
  exit(0);
}
