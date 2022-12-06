

int my_truncate(MINODE *mip)
{
    int i,j;
    int sibuf[256], dibuf[256];

   if(!mip){
    printf("Error: truncate failed, passed null pointer.\n");
    return -1;
   }

   for(i = 0; i < 12; i++){ // deallocs the first 12 direct blocks
        if(mip->INODE.i_block[i]){
            bdalloc(dev, mip->INODE.i_block[i]);
            mip->INODE.i_block[i] = 0;
        }
   }

   if(mip->INODE.i_block[12]){ // if single indirect block exists, dealloc each of its blocks
        get_block(dev, mip->INODE.i_block[12], sibuf);
        for(i = 0; i < 256; i++){
            if(sibuf[i]){
                bdalloc(dev, sibuf[i]);
            }
        }
        mip->INODE.i_block[12] = 0;
   }

   if(mip->INODE.i_block[13]){ // if double indirect4 block exists, dealloc each of its single indirect blocks
        get_block(dev, mip->INODE.i_block[13], dibuf);
        for(i = 0; i < 256; i++){
            if(dibuf[i]){
                get_block(dev, dibuf[i], sibuf);
                for(j = 0; j < 256; j++){
                    if(sibuf[j] != 0){
                        bdalloc(dev, sibuf[j]);
                    }
                }
            }
        }
        mip->INODE.i_block[13] = 0;
   }

    mip->INODE.i_atime = time(0L);
    mip->INODE.i_size = 0;
    mip->dirty = 1;

    return 0;
}


// filename, mode -> file descriptor
// RETURNS NEGATIVE VALUE ON FAILURE
int my_open(char *filename, int mode){
    //You may use mode = 0|1|2|3 for R|W|RW|APPEND
    int ino;
    MINODE *mip;
    OFT *oftp;
    int i;
    ino = getino(filename);
    if(!ino){ // ino doesn't exist, creat first
        mycreat(filename);
        ino = getino(filename);
        if(!ino){
            printf("Error: open failed.\n");
            return -1;
        }
    }

    if(mode == 0 || mode == 2){ // needs read perm
        if(!my_access(filename, READPERM)){
            printf("Error: invalid permissions.\n");
            return -1;
        }
    }
    if(mode == 1 || mode == 2 || mode == 3){ // needs write perm
        if(!my_access(filename, WRITEPERM)){
            printf("Error: invalid permissions.\n");
            return -1;
        }
    }

    mip = iget(dev, ino);

    if(!S_ISREG(mip->INODE.i_mode)){
        printf("Error: Cannot open file, file is not regular.\n");
        return -1;
    }
    
    
    for(i = 0; i < NFD; i++){
        if(!running->fd[i] || running->fd[i]->refCount == 0) continue; // skip unallocated fds
        if(running->fd[i]->minodePtr->ino == mip->ino){ // if the inodes match, check if its open for WR,RDWR,APPEND
            if(running->fd[i]->mode == 1 || running->fd[i]->mode == 2 || running->fd[i]->mode == 3){
                printf("Error: File already opened in an INCOMPATIBLE mode.\n");
                return -1;
            }
        }
    }
    for(i = 0; i < 64; i++){
        if(oft[i].refCount == 0) break;
    }
    if(i == 64){
        printf("Error: No available OFT.\n");
        return -1;
    }

    oftp = &oft[i];
    switch(mode){
         case 0 : oftp->offset = 0;     // R: offset = 0
                  break;
         case 1 : my_truncate(mip);        // W: truncate file to 0 size
                  oftp->offset = 0;
                  break;
         case 2 : oftp->offset = 0;     // RW: do NOT truncate file
                  break;
         case 3 : oftp->offset =  mip->INODE.i_size;  // APPEND mode
                  break;
         default: printf("Error: invalid mode\n");
                  return -1;
      }
    
    oftp->minodePtr = mip;
    oftp->refCount = 1;
    oftp->mode = mode;

    i = 0;
    for(i = 0; running->fd[i] && i < NFD; i++);
    if(i == NFD){
        printf("Error: No available fd in running process.\n");
        return -1;
    }
    running->fd[i] = oftp;
    mip->INODE.i_atime = time(0L);
    if(mode == 1 || mode == 2 || mode == 3){
        mip->INODE.i_mtime = time(0L);
    }
    mip->dirty = 1;

    return i;

}


int my_close_file(int fd){
    OFT *oftp;
    MINODE *mip;


    if(fd < 0 || fd >= NFD){
        printf("Error: fd is not in range.\n");
        return -1;
    }

    if(!running->fd[fd]){
        printf("Error: fd is not allocated.\n");
        return -1;
    }

    oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->refCount--;
    if(oftp->refCount > 0) return 0;
    mip = oftp->minodePtr;
    iput(mip);
    return 0;
}


// assume it is always SEEK_SET
int my_lseek(int fd, int position){
    OFT* oftp;
    MINODE *mip;
    int o_pos;

    if(fd < 0 || fd >= NFD){
        printf("Error: fd is not in range.\n");
        return -1;
    }

    if(!running->fd[fd]){
        printf("Error: fd is not allocated.\n");
        return -1;
    }

    oftp = running->fd[fd];

    if(position >= oftp->minodePtr->INODE.i_size){
        printf("Error: out of file bounds.\n");
        return -1;
    }

    o_pos = oftp->offset;
    oftp->offset = position;
    return o_pos;
}

int pfd(){
    int i;

    //You may use mode = 0|1|2|3 for R|W|RW|APPEND
    char* a;
    printf(" fd     mode    offset    INODE");
    printf("----    ----    ------   --------");
    for(i = 0; i < NFD; i++){
        if(running->fd[i]){
            printf("%8d", i);
            if(running->fd[i]->mode == 0)printf("%8s", "READ");
            if(running->fd[i]->mode == 1)printf("%8s", "WRITE");
            if(running->fd[i]->mode == 2)printf("%8s", "RDWR");
            if(running->fd[i]->mode == 3)printf("%8s", "APPEND");
            printf("%7d", running->fd[i]->offset);
            printf("[%d, %d]", running->fd[i]->minodePtr->dev, running->fd[i]->minodePtr->ino);
            printf("\n");
        }
    }
    printf("--------------------------------------\n");
}