

int my_read_file(char* buf){ // it doesn't show to use buf as input but i dont see how it wouldn't
    int fd, nbytes;
    scanf("read %d %d", fd, nbytes);
    if(fd < 0 || fd >= NFD || !running->fd[fd]){
        printf("Error: invalid fd.\n");
        return -1; // make sure this works as invalid return
    }
    if(running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2){
        printf("Error: fd not opened for read.\n");
        return -1;
    }
    return my_read(fd, buf, nbytes);
}

int my_read(int fd, char* buf, int nbytes){
    int read_bytes, available_bytes; // TOTAL bytes read and avaialable bytes in the file
    int sibuf[256], dibuf[256]; // single and double indirection block buffers
    char readbuf[BLKSIZE];
    char *cq, *cp; // cq is byte ptr to current place in buf, cp is byte ptr to current place in block
    OFT *oftp;
    MINODE *mip;
    int lbk, startByte; // logical block and startByte of the block
    int blk;
    int remain; // remaining bytes in a block

    oftp = running->fd[fd];
    mip = oftp->minodePtr;
    read_bytes = 0;
    available_bytes = mip->INODE.i_size - oftp->offset;
    cq = buf;
    while(available_bytes && nbytes){
        lbk = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;
        if (lbk < 12){                    // lbk is a direct block
            blk = mip->INODE.i_block[lbk]; // map LOGICAL lbk to PHYSICAL blk
        }
        else if (lbk >= 12 && lbk < 256 + 12) { //  indirect blocks 
            blk = lbk - 12; // skip the first 12 blocks
            get_block(dev, mip->INODE.i_block[12], sibuf);
            blk = sibuf[blk];
        }
        else{ //  double indirect block
            get_block(dev, mip->INODE.i_block[13], dibuf); // puts double indirect block in dbuf
            blk = dibuf[(lbk - (256 + 12)) / 256];
            get_block(dev, blk, sibuf); // puts correct single indirect block in sbuf
            blk = sibuf[(lbk - (256 + 12)) % 256];
        }

        //printf("reading block number %d\n", blk);

        get_block(dev, blk, readbuf);
        cp = readbuf + startByte;
        remain = BLKSIZE - startByte;
        if(remain == BLKSIZE && available_bytes >= BLKSIZE && nbytes >= BLKSIZE){ // try to read full block all at once for optimization
            memcpy(cq, cp, BLKSIZE);
            cq += BLKSIZE;
            oftp->offset += BLKSIZE;
            read_bytes += BLKSIZE;
            available_bytes -= BLKSIZE;
            nbytes -= BLKSIZE;
        }else{
            while (remain > 0){
            *cq++ = *cp++;             // copy byte from readbuf[] into buf[]
             oftp->offset++;           // advance offset 
             read_bytes++;                  // inc count as number of bytes read
             available_bytes--; nbytes--;  remain--;
             if (nbytes <= 0 || available_bytes <= 0) 
                 break;
            }
        }
        
    }
    return read_bytes;
}

void my_cat(char* filename){
    char mybuf[BLKSIZE];
    int n, i, fd;
    if(!getino(filename)){
        printf("Error: File doesn't exist.\n");
        return;
    }
    fd = my_open(filename, 0);
    while(n = my_read(fd, mybuf, BLKSIZE)){
        for(i = 0; i < n; i++)printf("%c", mybuf[i]);
    }
    my_close_file(fd);
}

