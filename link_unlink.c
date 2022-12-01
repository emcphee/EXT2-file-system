

void my_link(char* path1, char* path2){
    int oino, pino;
    MINODE *omip, *pmip;
    char dir[256], base[256], temp[256];
    // pathname = oldfile ... pathname2 = newfile
    oino = getino(path1);
    if(oino == 0){
        printf("Error: Can't file file to link.\n");
        return;
    }
    omip = iget(dev, oino);
    if(S_ISDIR(omip->INODE.i_mode)){
        printf("Error: Cannot link a dir.\n");
        iput(omip);
        return;
    }
    if(getino(path2)){
        printf("Error: File with that name already exists.\n");
        iput(omip);
        return;
    }

    pathname_to_dir_and_base(path2, dir, base);

    pino = getino(dir);
    pmip = iget(dev, pino);

    enter_name(pmip, oino, base);

    omip->INODE.i_links_count++;
    omip->dirty = 1;
    iput(omip);
    iput(pmip);

}

// theres problem where unlinking starting from root node breaks
void my_unlink(char* path){
    int ino, pino, i;
    MINODE *mip, *pmip;
    char dir[128], base[128], temp[256];
    
    ino = getino(path);
    if(!ino){
        printf("Error: file doesn't exist.\n");
        return;
    }
    mip = iget(dev, ino);
    if(!S_ISREG(mip->INODE.i_mode) && !S_ISLNK(mip->INODE.i_mode)){
        printf("Error: Cannot unlink that filetype.\n");
        iput(mip);
        return;
    }
    pathname_to_dir_and_base(path, dir, base);
    if(strlen(dir) == 0){
        pino = running->cwd->ino;
    }else{
        pino = getino(dir);
    }
    pmip = iget(dev, pino);
    rmchild(pmip, base);
    pmip->dirty = 1;
    iput(pmip);
    mip->INODE.i_links_count--;
    if(mip->INODE.i_links_count > 0){
        mip->dirty = 1;
    }else{ // links_count = 0 so we remove the file
        my_truncate(mip);
        idalloc(mip->dev, mip->ino);
    }
    iput(mip);
}