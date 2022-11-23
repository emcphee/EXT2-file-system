/*************** type.h file for LEVEL-1 ****************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp; // ext2_super_block pointer
GD    *gp; // ext2_group_desc pointer
INODE *ip; // ext2_inode pointer
DIR   *dp; // ext2_dir_entry_2 pointer

#define FREE        0
#define READY       1

#define BLKSIZE  1024
#define NMINODE   128
#define NPROC       2
#define NFD        16 //number of fd's

// memory index node
typedef struct minode{
  INODE INODE;           // INODE structure on disk
  int dev, ino;          // (dev, ino) of INODE
  int refCount;          // in use count
  int dirty;             // 0 for clean, 1 for modified

  int mounted;           // for level-3
  struct mntable *mptr;  // for level-3
}MINODE;

typedef struct oft{     // OpenFileTable
        int  mode;      // R|W|RW|APP
        int  refCount;
        MINODE *minodePtr;
        int  offset;
}OFT;  


// procedure
typedef struct proc{
  struct proc *next;
  int          pid;      // process ID  
  int          uid;      // user ID
  int          gid;
  MINODE      *cwd;      // CWD directory pointer  

  OFT    *fd[NFD];
}PROC;
