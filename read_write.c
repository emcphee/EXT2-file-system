

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


int my_write_file(){
    int fd, nbytes;
    char buf[1024]; // maybe this needs to be bigger
    scanf("write %d %s", fd, buf);
    if(fd < 0 || fd >= NFD || !running->fd[fd]){
        printf("Error: invalid fd.\n");
        return -1; // make sure this works as invalid return
    }
    if(running->fd[fd]->mode != 1 && running->fd[fd]->mode != 2 || running->fd[fd]->mode != 3){
        printf("Error: fd not opened for write.\n");
        return -1;
    }
    nbytes = strlen(buf); // ?
    return my_write(fd, buf, nbytes);
}

int my_write(int fd, char* buf, int nbytes){
    int blk, lbk, startByte, ibuf[256], ibuf2[256], remain;
    int indirect_block_num, indirect_block_offset;
    OFT *oftp;
    MINODE *mip;
    char wbuf[BLKSIZE], *cp, *cq;

    oftp = running->fd[fd];
    mip = oftp->minodePtr;

    cq = buf;

    while(nbytes > 0){
        lbk = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;

        if(lbk < 12){ // direct blocks
            if(mip->INODE.i_block[lbk] == 0){
                mip->INODE.i_block[lbk] = balloc(dev);
            }
            blk = mip->INODE.i_block[lbk];
        }else if(lbk >= 12 && lbk < 256 + 12){ // single indirect blocks
            lbk = lbk - 12; // ignore first 12 blocks
            if(mip->INODE.i_block[12] == 0){
                mip->INODE.i_block[12] = balloc(dev);
                bzero(wbuf, BLKSIZE);
                put_block(dev, mip->INODE.i_block[12], wbuf);
            }
            get_block(dev, mip->INODE.i_block[12], ibuf);
            blk = ibuf[lbk];
            if(blk == 0){ // need to allocate block
                ibuf[lbk] = balloc(dev);
                blk = ibuf[lbk];
                put_block(dev, mip->INODE.i_block[12], ibuf);
            }
        }else{ // double indirect blocks
            if(mip->INODE.i_block[13] == 0){
                mip->INODE.i_block[13] = balloc(dev);
                bzero(wbuf, BLKSIZE);
                put_block(dev, mip->INODE.i_block[13], wbuf);
            }
            get_block(dev, mip->INODE.i_block[13], ibuf);

            lbk = lbk - (256 + 12); // ignore first 12 direct and first 256 indirect
            indirect_block_num = lbk / BLKSIZE;
            indirect_block_offset = lbk % BLKSIZE;

            if(ibuf[indirect_block_num] == 0){
                ibuf[indirect_block_num] = balloc(dev);
                bzero(wbuf, BLKSIZE);
                put_block(dev, ibuf[indirect_block_num], wbuf);
                put_block(dev, mip->INODE.i_block[13], ibuf);
            }

            get_block(dev, ibuf[indirect_block_num], ibuf2); // correct single indirection block now in ibuf2

            if(ibuf2[indirect_block_offset] == 0){
                ibuf2[indirect_block_offset] = balloc(dev);
                put_block(dev, ibuf[indirect_block_num], ibuf2);
            }
            blk = ibuf2[indirect_block_offset];
        }
        
        get_block(dev, blk, wbuf);
        cp = wbuf + startByte;
        remain = BLKSIZE - startByte;
        
        // TODO need to write optimized code for this!!!
            while (remain > 0 && nbytes > 0){               // write as much as remain allows  
                *cp++ = *cq++;              // cq points at buf[ ]
                nbytes--; remain--;         // dec counts
                oftp->offset++;             // advance offset
                if (oftp->offset > mip->INODE.i_size)  // especially for RW|APPEND mode
                    mip->INODE.i_size++;    // inc file size (if offset > fileSize)
            }
        put_block(dev, blk, wbuf);

    }
    mip->dirty = 1;
    mip->INODE.i_atime = time(0L);
    mip->INODE.i_mtime = time(0L);
    return nbytes;
}

int my_cp(char* src, char* dst){
    char buf[BLKSIZE];
    int s_fd, d_fd;
    if(!src || !dst){
        printf("Error: Invalid input for cp.\n");
        return -1;
    }

    if(!getino(src)){
        return -1;
    }

    s_fd = my_open(src, 0);
    d_fd = my_open(dst, 1);
    if(s_fd < 0 || d_fd < 0){
        printf("Erorr: cp failed to open files.\n");
        return -1;
    }

    while(n = my_read(s_fd, buf, BLKSIZE)){
        my_write(d_fd, buf, n);
    }
    my_close_file(s_fd);
    my_close_file(d_fd);
    return 0;
}