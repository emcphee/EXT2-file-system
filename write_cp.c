

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
        
        if(nbytes >= BLKSIZE && remain == BLKSIZE){
            memcpy(cp, cq, BLKSIZE);
            nbytes -= BLKSIZE;
            oftp->offset += BLKSIZE;
            if(oftp->offset > mip->INODE.i_size) mip->INODE.i_size = oftp->offset;
        }else{
            while (remain > 0 && nbytes > 0){               // write as much as remain allows  
                *cp++ = *cq++;              // cq points at buf[ ]
                nbytes--; remain--;         // dec counts
                oftp->offset++;             // advance offset
                if (oftp->offset > mip->INODE.i_size)  // especially for RW|APPEND mode
                    mip->INODE.i_size++;    // inc file size (if offset > fileSize)
            }
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




int my_mv(char* src, char* dst){
    int sino, dino;
    MINODE *smip, *dmip;

    sino = getino(src);
    if(!sino){
        printf("Error: Source doesn't exist.\n");
        return -1;
    }

    dino = getino(dst);
    if(!dino){
        mycreat(dst);
        dino = getino(dst);
    }

    smip = iget(dev, sino);
    dmip = iget(dev, dino);
    if(smip->dev == dmip->dev){ // same dev
        my_unlink(dst);
        my_link(src, dst);
        my_unlink(src);
    }else{ // different dev
        my_cp(src, dst);
        my_unlink(src);
    }
    iput(smip);
    iput(dmip);
}
