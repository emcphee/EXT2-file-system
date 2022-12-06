



MOUNT* getmptr(int d){
  int i;
  for(i = 0; i < 8; i++){
    if(mountTable[i].dev == d) return &mountTable[i];
  }
  return 0;
}

void display_mounts(){
    int i;
    for(i = 0; i < 8; i++){
        if(mountTable[i].dev == 0) continue;
        printf("mountTable[%d]:\n", i);
        printf("Mount name = %s\n", mountTable[i].mount_name);
        printf("dev = %d\n\n", mountTable[i].dev);
    }
}

int mount(char* mnt_disk, char* mnt_point){
  int i, im, ffd, ino;
  MINODE* mip;
  char buf[BLKSIZE];
  SUPER *sp_blk;
  GD *gd_blk;
  MOUNT* mntPtr;
    if(!mnt_disk[0] || !mnt_point[0]){
        display_mounts();
        return;
    }

    // check if already mounted
    for(i = 0; i < 8; i++){
      if(mountTable[i].dev != 0 && !strcmp(mnt_disk, mountTable[i].name)){
        printf("Error: filesystem already mounted.\n");
        return -1;
      }
    }

    mntPtr = 0;
    for(im = 0; im < 8; im++){
      if(mountTable[im].dev == 0){
          mntPtr = &mountTable[im];
          break;
      }
    }

    if(mntPtr == 0){
      printf("Error: No available mount table entries\n");
      return -1;
    }

    ffd = open(mnt_disk, O_RDWR);
    if(ffd < 0){
      printf("Error: couldn't open filesystem.\n");
      return -1;
    }

    get_block(ffd, 1, buf);
    sp_blk = (SUPER*) buf;
    if(sp_blk->s_magic != 0xEF53){
      printf("Error: Not an EXT2 filesystem.\n");
      close(ffd);
      return -1;
    }

    ino = getino(mnt_point);
    mip = iget(dev, ino);
    
    if(mip->refCount > 1){
      printf("Error: mount point is busy.\n");
      iput(mip);
      close(ffd);
      return -1;
    }

    if(!S_ISDIR(mip->INODE.i_mode)){
      printf("Error: mount point is not a dir.\n");
      iput(mip);
      close(ffd);
      return -1;
    }

    mntPtr->dev = ffd;
    mntPtr->ninodes = sp_blk->s_inodes_count;
    mntPtr->nblocks = sp_blk->s_blocks_count;

    get_block(mntPtr->dev, 2, buf);
    gd_blk = (GD*) buf;
    mntPtr->bmap = gd_blk->bg_block_bitmap;
    mntPtr->imap = gd_blk->bg_inode_bitmap;
    mntPtr->iblk = gd_blk->bg_inode_table;
    mntPtr->mounted_inode = mip;

    mip->mounted = 1;
    mip->mptr = mntPtr;
    
    
    strcpy(mntPtr->mount_name, mnt_point);
    strcpy(mntPtr->name, mnt_disk);
    
    return 0;
}

int umount(char *filesys){
    int i,j;
    int found = 0;
    MOUNT* mptr;
    MINODE* mip;
    for(i = 0; i < 8; i++){
      if(!strncmp(filesys, mountTable[i].mount_name, 128)){
        found = 1;
        break;
      }
    }
    if(!found){
      printf("Error: filesystem not mounted.\n");
      return -1;
    }
    mptr = &mountTable[i];

    for(j = 0; j < NMINODE; j++){
      if(minode[j].dev == mptr->dev && minode[j].refCount > 0){
        printf("Error: minode is busy.\n");
        return -1;
      }
    }
    
    mip = mptr->mounted_inode;
    mip->mounted = 0;


    //mptr->dev = 0; // this line needs to be here to free the mount entry but it breaks everything for some reason :(
    mptr->mount_name[0] = 0;
    mptr->name[0] = 0;

    close(mptr->dev);
    iput(mip);

    return 0;

}