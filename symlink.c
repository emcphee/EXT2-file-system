

void my_symlink(){
    char dir[128], base[128], temp[256];
    DIR* dp;
    MINODE *pmip, *mip;
    int ino, pino;

    //swaps pathname and pathname2 to call creat
    strcpy(temp, pathname);
    strcpy(pathname, pathname2);
    strcpy(pathname2, temp);
    // now newfilename = pathname
    // and oldfilename = pathname2
    mycreat();
    ino = getino(pathname);
    if(!ino){
        printf("Error: Could not create link.\n");
        return;
    }
    printf("ino = %d\n", ino);
    mip = iget(dev, ino);
    pathname_to_dir_and_base(pathname, dir, base);
    if(strlen(dir) == 0){
        pino = running->cwd->ino;
    }else{
        pino = getino(dir);
    }
    pmip = iget(dev, pino);
    printf("pino = %d\n", pino);
    pmip = iget(dev, pino);
    pmip->INODE.i_links_count++;
    pmip->dirty = 1;
    iput(pmip);
    mip->INODE.i_mode = 0xA1A4;
    mip->INODE.i_size = strlen(pathname2);
    strcpy(mip->INODE.i_block, pathname2);
    iput(mip);

}

// returns 0 on failure
int my_readlink(char* filepath, char* buffer){
    MINODE *mip;
    int ino;

    ino = getino(pathname);
    if(!ino){
        printf("Error: file doesn't exist.\n");
        return 0;
    }
    mip = iget(dev, ino);
    if(!S_ISLNK(mip->INODE.i_mode)){
        printf("Error: file is not a link.\n");
        iput(mip);
        return 0;
    }
    // file is known to be a symbolic link so we can just copy its contents into buffer
    strcpy(buffer, mip->INODE.i_block);
    iput(mip);
    return strlen(buffer); // not sure why we return size here. If buffer is not supossed to be null term'd fix it.
}