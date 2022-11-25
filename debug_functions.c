

// REMEMBER blocks start at 1, so subtract 1 from block number to check against bitmap
void pretty_print_block_bitmap_to_file(){
    int i, t;
    char buf[BLKSIZE];
    FILE* fp = fopen("debug_outputs/bmap.txt", "w");
    get_block(dev, bmap, buf);
    fprintf(fp, "          0   1   2   3   4   5   6   7   8   9\n");
    fprintf(fp, "-----------------------------------------------\n");
    for(i = 0; i < nblocks; i++){
        if(i % 10 == 0) fprintf(fp, "%4d : ", i);
        t = tst_bit(buf, i);
        if(t){ // t is not in 0/1 form but 0/not 0 form of a bool, so this is just a dirty fix
            fprintf(fp, "%4d", 1);
        }else{
            fprintf(fp, "%4d", 0);
        }
        if(i % 10 == 9) fprintf(fp, "\n");
    }
    fclose(fp);
}

void print_block_bitmap_to_file_binary(){
    int i, t;
    char buf[BLKSIZE];
    FILE* fp = fopen("debug_outputs/bmapbin.txt", "w");
    get_block(dev, bmap, buf);
    for(i = 0; i < nblocks; i++){
        t = tst_bit(buf, i);
        if(t){ // t is not in 0/1 form but 0/not 0 form of a bool, so this is just a dirty fix
            fprintf(fp, "1,");
        }else{
            fprintf(fp, "0,");
        }
    }
    fclose(fp);
}

// prints all blocknums allocated to a file to a file comma-sep'd
void print_file_block_nums_to_file(MINODE* mip, char* filename){
    int i, j;
    INODE* ip;
    int sibuf[256], dibuf[256];
    char path[256];
    FILE* fp;
    strcpy(path, "debug_outputs/");
    strcat(path, filename);
    fp = fopen(path, "w");
    ip = &mip->INODE;
    for(i = 0; i < 12; i++){ // direct blocks
        if(ip->i_block[i]) fprintf(fp, "%d,", ip->i_block[i]);
    }

    if(ip->i_block[12]){
        get_block(dev, ip->i_block[12], sibuf);
        for(i = 0; i < 256; i++){
            if(sibuf[i]) fprintf(fp, "%d,", sibuf[i]);
        }
        fprintf(fp, "%d,", ip->i_block[12]);
    }

    if(ip->i_block[13]){
        get_block(dev, ip->i_block[13], dibuf);
        for(i = 0; i < 256; i++){
            if(dibuf[i]){
                get_block(dev, dibuf[i], sibuf);
                for(j = 0; j < 256; j++){
                    if(sibuf[j]) fprintf(fp, "%d,", sibuf[j]);
                }
            }
        }
    }

    fclose(fp);
}

void print_blocks(MINODE* mip){
    int i;
    int ibuf[256];
    char* b;
    printf("DIBLOCK = %d\nContents = ", mip->INODE.i_block[13]);
    if(mip->INODE.i_block[13]){
        get_block(dev, mip->INODE.i_block[13], ibuf);
        b = ibuf;
        for(i = 0; i < 1024; i++){
            printf("%c", b[i]);
        }
    }
    printf("\n");

    /*
    int i,j;
    int ibuf[256], ibuf2[256];
    printf("DIRECT: ");
    for(i = 0; i < 12; i++){
        if(mip->INODE.i_block[i])printf("%d ", mip->INODE.i_block[i]);
    }
    printf("\n");
    if(mip->INODE.i_block[12]){
        printf("INDIRECT (indblock = %d): ",mip->INODE.i_block[12]);
        get_block(dev, mip->INODE.i_block[12], ibuf);
        for(i = 0; i < 256; i++){
            if(ibuf[i]) printf("%d ", ibuf[i]);
        }
        printf("\n");
    }
    */
    /*
    if(mip->INODE.i_block[13]){
        printf("DOUBLE INDIRECT (dindblock = %d", mip->INODE.i_block[13]);
        get_block(dev, mip->INODE.i_block[13], ibuf);
        for(i = 0; i < 256; i++){
            if(ibuf[i]){
                printf("SUB-INDIRECT (indblock = %d): ",ibuf[i]);
                get_block(dev, ibuf[i], ibuf2);
                for(j = 0; j < 256; j++){
                    if(ibuf2[j]){
                        printf("%d ", ibuf2[j]);
                    }
                }
                printf("\n");
            }
        }
        printf("\n");
    }
    */
    

}