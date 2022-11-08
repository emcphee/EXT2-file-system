

int check_empty(MINODE *mip){
  char buf[BLKSIZE];
  char *cp;
  DIR *dp;

  char dirname[64];

  if(mip->INODE.i_links_count > 2){
    return 0;
  } else if(mip->INODE.i_links_count == 2){
    for(int i=0; i<12; i++){
      if(mip->INODE.i_block[i] == 0){
        break;
      }

      get_block(mip->dev, mip->INODE.i_block[i], buf);
      dp = (DIR *) buf;
      cp = buf;

      while(cp < buf + BLKSIZE){
        strncpy(dirname, dp->name);
        dirname[dp->name_len] = 0;
        if(strcmp(dirname, ".") && strcmp(dirname, "..")){
          return 0;
        }
        cp += dp->rec_len;
        dp = (DIR *)cp;
      }
    }
  }
  return 1;
}

void rmdir(){

}
