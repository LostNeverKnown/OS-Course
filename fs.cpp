#include <iostream>
#include <iomanip>
#include "fs.h"

int current_block = ROOT_BLOCK;//the block you stand on currently
std::string current_path = "/";//describes the path for current_block

FS::FS()
{

}

FS::~FS()
{

}

// formats the disk, i.e., creates an empty file system
int
FS::format()
{
    root = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    current_block = ROOT_BLOCK;//block you stand on is now root
    current_path = "/";

    for(int i = 0; i < BLOCK_SIZE/2; i++){
      if(i == 0)
        fat[ROOT_BLOCK] = FAT_EOF;
      else if(i == 1)
        fat[FAT_BLOCK] = FAT_EOF;
      else
        fat[i] = FAT_FREE;
    }
    for(int i = 0; i < no_dir; i++){
      root[i].size = 0;
      for(int k = 0; k < 56; k++)
        root[i].file_name[k] = '\0';//fill name array with end of file to easier show if they are in use or not 
                                    //and the char array is easier to change into a string
    }

    disk.write(FAT_BLOCK, (uint8_t*) fat);
    disk.write(ROOT_BLOCK, (uint8_t*) root);
    free(root);
    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int
FS::create(std::string filepath)
{
    if(filepath.length() > 55){
      std::cout << "File name too long!" << std::endl;
      return 0;
    }
    root = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    disk.read(current_block, (uint8_t*) root);
    disk.read(FAT_BLOCK, (uint8_t*) fat);

    //checks if filepath name exists in current dir
    int entry, block, size = 0;
    bool check = find_file(root, filepath, entry);
    if(check){
      std::cout << "File name exists already in current dir!" << std::endl;
      free(root);
      return 0;
    }
    //find an empty spot in dir
    entry = find_empty_entry(root);
    //no empty spot found in dir
    if(entry == -1){
      std::cout << "There is no room for more files in this dir!" << std::endl;
      free(root);
      return 0;
    }
    //find free block in FAT
    block = find_empty_block();
    //no empty spot found in FAT
    if(block == -1){
      std::cout << "There is no room in FAT!" << std::endl;
      free(root);
      return 0;
    }
    root[entry].first_blk = block;
    //get input from user
    std::string input, file, temp = "";
    while(getline(std::cin, input)){
      //if input is just enter
      if(input.empty())
        break;
      if(file != "")
        file.append("\n");
      temp = file;
      //if file gets larger than one disk block
      if(temp.append(input).size() > BLOCK_SIZE){
        temp = "";
        int to_take = BLOCK_SIZE - file.size();//what needs to be added so file gets to BLOCK_SIZE in size
        for(int i = 0; i < input.length(); i++){
          if(i < to_take)//add text to file till file.size() gets to BLOCK_SIZE
            file += input[i];
          else
            temp += input[i];
        }
        disk.write(block, (uint8_t*)file.c_str());
        size += file.size();
        file = temp;
        int old_block = block;
        //search for a free block to place remaining file
        block = 1;
        for(int i = 2; i < BLOCK_SIZE/2; i++){
          block++;
          if(fat[block] == FAT_FREE && block != old_block)
            i = BLOCK_SIZE/2;
        }
        fat[old_block] = block;
        disk.write(FAT_BLOCK, (uint8_t*) fat);
      }
      else{
        file.append(input);
      }
    }
    //fill dir_entry for the file
    int count = 0;
    for(int i = 0; i < filepath.length(); i++){
      if(filepath[i] != ' '){
        root[entry].file_name[count] = filepath[i];
        count++;
      }
    }
    size += file.size()+1;
    root[entry].size = size;
    root[entry].type = TYPE_FILE;
    root[entry].access_rights = (READ | WRITE);
    fat[block] = FAT_EOF;
    //write/update disk
    disk.write(current_block, (uint8_t*) root);
    disk.write(block, (uint8_t*)file.c_str());
    disk.write(FAT_BLOCK, (uint8_t*) fat);
    free(root);
    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    root = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    int entry, block, check, type;
    check = follow_path(filepath, block, entry, type);

    std::string file;
    bool loop = true;
    disk.read(block, (uint8_t*) root);
    disk.read(FAT_BLOCK, (uint8_t*) fat);
    //check if path could be followed
    if(check == -1){
      std::cout << "Can't follow path!" << std::endl;
      free(root);
      return 0;
    }
    else if(type == TYPE_DIR){//check if dir
      std::cout << "Can't cat a dir!" << std::endl;
      free(root);
      return 0;
    }
    else if(!req_read(root[entry].access_rights)){//check READ access_rights
      std::cout << "This file does not have read accessrights!" << std::endl;
      free(root);
      return 0;
    }
    //found everything tht is needed
    block = root[entry].first_blk;
    char char_temp[BLOCK_SIZE];
    while(loop){//read the text and add to file and repeat till file hit FAT_EOF
      disk.read(block, (uint8_t*) char_temp);
      file.append(char_temp);
      if(fat[block] == FAT_EOF)
        loop = false;
      else
        block = fat[block];
    }
    std::cout << file << std::endl;

    free(root);
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls()
{
    root = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    disk.read(current_block, (uint8_t*) root);

    std::string filepath, type, rights;
    std::cout << "name" << std::setw(10) << "type" << std::setw(15) << "accessrights" << std::setw(15) << "size" << std::endl;
    for(int i = 0; i < no_dir; i++){
      rights = "---";
      if(root[i].file_name[0] != '\0' && root[i].file_name[0] != '.'){
        //name
        filepath = root[i].file_name;
        //type
        if(root[i].type == TYPE_FILE)
          type = "file";
        else
          type = "dir";
        //accessrights
        int rights_num = root[i].access_rights;
        if(rights_num == 4 || rights_num == 5 || rights_num == 6 || rights_num == 7)
          rights[0] = 'r';
        if(rights_num == 2 || rights_num == 3 || rights_num == 6 || rights_num == 7)
          rights[1] = 'w';
        if(rights_num == 1 || rights_num == 3 || rights_num == 5 || rights_num == 7)
          rights[2] = 'x';
        //size and print
        if(root[i].size == 0)
          std::cout << filepath << std::setw(10) << type << std::setw(15) << rights << std::setw(15) << "-" << std::endl;
        else
          std::cout << filepath << std::setw(10) << type << std::setw(15) << rights << std::setw(15) << root[i].size << std::endl;
      }
    }

    free(root);
    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(std::string sourcepath, std::string destpath)
{
    disk.read(FAT_BLOCK, (uint8_t*) fat);
    trim(destpath);
    trim(sourcepath);
    std::string source_name;
    int last = 0;
    find_last(sourcepath, last, '/');
    //source_name for the file getting copied
    if(sourcepath[0] != '/' && last == 0)
      source_name = sourcepath;
    else
      source_name = sourcepath.substr(last+1, sourcepath.length());
    //sourcepath
    root = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    int source_entry, source_block, source_check, source_type;
    source_check = follow_path(sourcepath, source_block, source_entry, source_type);
    //check if path could be followed
    if(source_check == -1){
      std::cout << "Can't follow sourcepath!" << std::endl;
      free(root);
      return 0;
    }
    else if(source_type == TYPE_DIR){//check if dir
      std::cout << "Can't copy a dir!" << std::endl;
      free(root);
      return 0;
    }
    disk.read(source_block, (uint8_t*) root);
    if(!req_read(root[source_entry].access_rights)){//check if READ access_rights
      std::cout << "Sourcepath has no read accessrights!" << std::endl;
      free(root);
      return 0;
    }
    //destpath
    dir_entry* dest_dir;
    dest_dir = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    int dest_entry, dest_block, dest_check, dest_type;
    std::string dest_name;
    find_last(destpath, last, '/');
    //dest_name for copied file name
    if(destpath[0] != '/' && last == 0)
      dest_name = destpath;
    else
      dest_name = destpath.substr(last+1, destpath.length());
    //check destination name length
    if(dest_name.length() > 55){
      std::cout << "Destination name too long!" << std::endl;
      free(root);
      free(dest_dir);
      return 0;
    }
    //check destpath
    dest_check = follow_path(destpath, dest_block, dest_entry, dest_type);
    if(dest_check == -1 && dest_type == TYPE_DIR){//could not find path before last name in destpath
      std::cout << "Could not follow destpath!" << std::endl;
      free(root);
      free(dest_dir);
      return 0;
    }
    else if(dest_check == 0 && dest_type == TYPE_FILE){//found existing file
      std::cout << "Destination already exists!" << std::endl;
      free(root);
      free(dest_dir);
      return 0;
    }
    disk.read(dest_block, (uint8_t*) dest_dir);
    if(dest_type == TYPE_DIR && source_block == dest_block){//cant copy to same dir without other name or else there are 2 files with same name in same dir
      std::cout << "Can not copy to same dir without a unique name!" << std::endl;
      free(root);
      free(dest_dir);
      return 0;
    }
    else if(dest_type == TYPE_DIR)//new file gets source file name
      dest_name = source_name;
    //find empty spot in dest_dir and FAT for new file
    int dest_file_block = find_empty_block();
    dest_entry = find_empty_entry(dest_dir);
      if(dest_entry == -1 || dest_file_block == -1){//no empty space in FAT or dir
        std::cout << "Can't copy when dir/FAT is full!" << std::endl;
        free(root);
        free(dest_dir);
        return 0;
      }
    //copy block to another block
    bool loop = true;
    char char_temp[BLOCK_SIZE];
    while(loop){
      disk.read(root[source_entry].first_blk, (uint8_t*) char_temp);
      disk.write(dest_file_block, (uint8_t*) char_temp);//copy
      if(fat[source_block] == FAT_EOF)//end
        loop = false;
      else{//not end
        //search for a free block to place remaining file
        int old_block = dest_file_block;
        dest_file_block = 1;
        for(int i = 2; i < BLOCK_SIZE/2; i++){
          dest_file_block++;
          if(fat[dest_file_block] == FAT_FREE && dest_file_block != old_block)
            i = BLOCK_SIZE/2;
        }
        fat[old_block] = dest_file_block;
        disk.write(FAT_BLOCK, (uint8_t*) fat);
        disk.read(FAT_BLOCK, (uint8_t*) fat);
        source_block = fat[source_block];
      }
    }
    //fill dir_entry for the destination file
    for(int i = 0; i < dest_name.length(); i++)
      dest_dir[dest_entry].file_name[i] = dest_name[i];
    dest_dir[dest_entry].size = root[source_entry].size;
    dest_dir[dest_entry].first_blk = dest_file_block;
    dest_dir[dest_entry].type = TYPE_FILE;
    dest_dir[dest_entry].access_rights = (READ+WRITE);
    fat[dest_file_block] = FAT_EOF;
    //write to block or update disk
    disk.write(dest_block, (uint8_t*) dest_dir);
    disk.write(FAT_BLOCK, (uint8_t*) fat);

    free(root);
    free(dest_dir);
    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(std::string sourcepath, std::string destpath)
{
    disk.read(FAT_BLOCK, (uint8_t*) fat);
    trim(destpath);
    //sourcepath
    root = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    int source_entry, source_block, source_check, source_type;
    source_check = follow_path(sourcepath, source_block, source_entry, source_type);
    if(source_check == -1){
      std::cout << "Can't follow sourcepath!" << std::endl;
      free(root);
      return 0;
    }
    else if(source_type == TYPE_DIR){
      std::cout << "Can't move a dir!" << std::endl;
      free(root);
      return 0;
    }
    disk.read(source_block, (uint8_t*) root);
    //destpath
    dir_entry* dest_dir;
    dest_dir = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    int dest_entry, dest_block, dest_check, dest_type;
    //if destpath is just root dir
    if(destpath == "/"){
      dest_block = ROOT_BLOCK;
      disk.read(dest_block, (uint8_t*) dest_dir);
    }
    else{//if not just root path
      dest_check = follow_path(destpath, dest_block, dest_entry, dest_type);
      if(dest_check == -1 && dest_type == TYPE_DIR || dest_check == -1 && source_block != dest_block){
        //could not find path before last name in destpath or could not find path and it is not a rename for source file
        std::cout << "Could not follow destpath!" << std::endl;
        free(root);
        free(dest_dir);
        return 0;
      }
      else if(dest_check == 0 && dest_type == TYPE_FILE){//found existing file
        std::cout << "There already exists a file with destination name!" << std::endl;
        free(root);
        free(dest_dir);
        return 0;
      }
      disk.read(dest_block, (uint8_t*) dest_dir);
    }
    //source and dest are in the same folder so only change file name
    if(source_block == dest_block){
      std::string dest_name;
      int last = 0;
      find_last(destpath, last, '/');
      //dest_name for source file rename
      if(destpath[0] != '/' && last == 0)
        dest_name = destpath;
      else
        dest_name = destpath.substr(last+1, destpath.length());
      //check destination name length
      if(dest_name.length() > 55){
        std::cout << "Destination name too long!" << std::endl;
        free(root);
        free(dest_dir);
        return 0;
      }

      for(int i = 0; i < 56; i++){
        if(i < dest_name.length())
          root[source_entry].file_name[i] = dest_name[i];
        else
          root[source_entry].file_name[i] = '\0';
      }
      disk.write(source_block, (uint8_t*) root);
      free(root);
      free(dest_dir);
      return 0;
    }
    //find an empty spot in dest_dir for moved source file
    dest_entry = find_empty_entry(dest_dir);
    if(dest_entry == -1){
      std::cout << "Destination folder is full, can not move file!" << std::endl;
      free(root);
      free(dest_dir);
      return 0;
    }
    //fill entry
    std::string temp = root[source_entry].file_name;
    for(int i = 0; i < temp.length(); i++){
      dest_dir[dest_entry].file_name[i] = temp[i];
      root[source_entry].file_name[i] = '\0';
    }
    dest_dir[dest_entry].size = root[source_entry].size;
    dest_dir[dest_entry].first_blk = root[source_entry].first_blk;
    dest_dir[dest_entry].type = TYPE_FILE;
    dest_dir[dest_entry].access_rights = root[source_entry].access_rights;
    //write to block or update disk
    disk.write(source_block, (uint8_t*) root);
    disk.write(dest_block, (uint8_t*) dest_dir);

    free(root);
    free(dest_dir);
    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int
FS::rm(std::string filepath)
{
    root = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    disk.read(FAT_BLOCK, (uint8_t*) fat);

    int check, block, entry, type = -1;
    check = follow_path(filepath, block, entry, type);
    disk.read(block, (uint8_t*) root);
    if(check == -1){
      std::cout << "Could not follow filepath!" << std::endl;
      free(root);
      return 0;
    }
    else if(block == ROOT_BLOCK && entry == -1){//trying to remove root or not found entry in root
      std::cout << "Can not remove root dir!" << std::endl;
      free(root);
      return 0;
    }
    else if(type == TYPE_DIR){//check if dir
      for(int i = 1; i < no_dir; i++){
        if(root[i].file_name[0] != '\0'){//not empty dir
          std::cout << "Can not remove a dir that is not empty!" << std::endl;
          free(root);
          return 0;
        }
      }
      block = root[0].first_blk;
      disk.read(block, (uint8_t*) root);
      std::string name;
      int last = 0;
      //get name to search for in parent dir
      find_last(filepath, last, '/');
      if(filepath[0] != '/' && last == 0)
        name = filepath;
      else
        name = filepath.substr(last+1, filepath.length());
      //search in parent dir
      find_file(root, name, entry);
      fat[root[entry].first_blk] = FAT_FREE;
      for(int i = 0; i < 56; i++)
        root[entry].file_name[i] = '\0';
    }
    else if(type == TYPE_FILE){//if file just remove
      int file_block, new_block = root[entry].first_blk;
      while(fat[new_block] != FAT_EOF){//set fat block free, for bigger files that take multiple blocks
        new_block = fat[new_block];
        fat[file_block] = FAT_FREE;
        file_block = new_block;
      }
      fat[root[entry].first_blk] = FAT_FREE;
      fat[new_block] = FAT_FREE;
      for(int i = 0; i < 56; i++)//delete name used for identification
        root[entry].file_name[i] = '\0';
    }

    disk.write(block, (uint8_t*) root);
    disk.write(FAT_BLOCK, (uint8_t*) fat);
    free(root);
    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int
FS::append(std::string filepath1, std::string filepath2)
{
    disk.read(FAT_BLOCK, (uint8_t*) fat);
    //sourcepath
    root = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    int source_entry, source_block, source_check, source_type;
    source_check = follow_path(filepath1, source_block, source_entry, source_type);
    if(source_check == -1){
      std::cout << "Can't follow sourcepath!" << std::endl;
      free(root);
      return 0;
    }
    else if(source_type == TYPE_DIR){
      std::cout << "Can not append from a dir!" << std::endl;
      free(root);
      return 0;
    }
    disk.read(source_block, (uint8_t*) root);
    if(!req_read(root[source_entry].access_rights)){
      std::cout << "Sourcepath does not have read accessrights!" << std::endl;
      free(root);
      return 0;
    }
    //destpath
    dir_entry* dest_dir;
    dest_dir = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    int dest_entry, dest_block, dest_check, dest_type;
    dest_check = follow_path(filepath2, dest_block, dest_entry, dest_type);

    if(dest_check == -1){
      std::cout << "Could not follow destpath!" << std::endl;
      free(root);
      free(dest_dir);
      return 0;
    }
    else if(dest_type == TYPE_DIR){
      std::cout << "Can not append to a dir!" << std::endl;
      free(root);
      free(dest_dir);
      return 0;
    }
    disk.read(dest_block, (uint8_t*) dest_dir);
    if(!req_write(dest_dir[dest_entry].access_rights)){
      std::cout << "Destpath does not have write accesstights!" << std::endl;
      free(root);
      free(dest_dir);
      return 0;
    }
    //find where filepath2 ends
    source_block = root[source_entry].first_blk;
    int dest_file_block = dest_dir[dest_entry].first_blk;
    bool loop = true;
    while(loop){
      if(fat[dest_file_block] == FAT_EOF)
        loop = false;
      else
        dest_file_block = fat[dest_file_block];
    }
    //append
    std::string file1, file2, temp = "";
    char char_file1[BLOCK_SIZE];
    char char_file2[BLOCK_SIZE];
    disk.read(source_block, (uint8_t*) char_file1);
    disk.read(dest_file_block, (uint8_t*) char_file2);
    file1 = char_file1;
    file2 = char_file2;
    file2.append("\n");//new line after destination text ends
    temp.append(file2);
    temp.append(file1);
    loop = true;
    if(temp.size() <= BLOCK_SIZE && fat[source_block] == FAT_EOF)//if append is smaller than BLOCK_SIZE
      disk.write(dest_file_block, (uint8_t*) temp.c_str());
    else{
      int fill = BLOCK_SIZE - (dest_dir[dest_entry].size%BLOCK_SIZE);//what is left to get file2 to size BLOCK_SIZE
      int old_block;
      while(loop){
        temp = "";
        for(int i = 0; i < BLOCK_SIZE; i++){
          if(i <= fill)//fill filepath2 block to BLOCK_SIZE
            file2 += file1[i];
          else
            temp += file1[i];
        }
        //search for a free block to place remaining file
        old_block = dest_file_block;
        dest_file_block = 1;
        for(int i = 2; i < BLOCK_SIZE/2; i++){
          dest_file_block++;
          if(fat[dest_file_block] == FAT_FREE && dest_file_block != old_block)
            i = BLOCK_SIZE/2;
        }
        fat[old_block] = dest_file_block;
        fat[dest_file_block] = FAT_EOF;
        //update
        disk.write(old_block, (uint8_t*) file2.c_str());
        disk.write(dest_file_block, (uint8_t*) temp.c_str());
        disk.write(FAT_BLOCK, (uint8_t*) fat);
        //is there more?
        if(fat[source_block] == FAT_EOF)
          loop = false;
        else{//if there is more
          file2 = temp;
          file2.append("\n");
          source_block = fat[source_block];
          disk.read(source_block, (uint8_t*) char_file1);
          file1 = char_file1;
          temp = "";
          temp.append(file2);
          temp.append(file1);
          if(temp.size() <= BLOCK_SIZE){
            disk.write(dest_file_block, (uint8_t*) temp.c_str());
            loop = false;
          }
        }
      }
    }

    dest_dir[dest_entry].size = dest_dir[dest_entry].size + root[source_entry].size;//update size
    disk.write(dest_block, (uint8_t*) dest_dir);
    disk.write(FAT_BLOCK, (uint8_t*) fat);
    free(root);
    free(dest_dir);
    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int
FS::mkdir(std::string dirpath)
{
    root = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    disk.read(FAT_BLOCK, (uint8_t*) fat);
    //find name for dir
    std::string name;
    int last;
    find_last(dirpath, last, '/');
    //dest_name for copied file name
    if(dirpath[0] != '/' && last == 0)
      name = dirpath;
    else
      name = dirpath.substr(last+1, dirpath.length());
    //check destination name length
    if(name.length() > 55){
      std::cout << "Destination name too long!" << std::endl;
      free(root);
      return 0;
    }

    int check, block, entry, type;
    check = follow_path(dirpath, block, entry, type);
    //checks if name exists
    if(check == -1 && type == TYPE_DIR){//type == TYPE_DIR says that the path could not be followed even when the name for the new dir is picked away from dirpath
      std::cout << "Could not follow path!" << std::endl;
      free(root);
      return 0;
    }
    else if(check == 0){
      std::cout << "Dir already exists!" << std::endl;
      free(root);
      return 0;
    }
    disk.read(block, (uint8_t*) root);
    //find empty entry
    entry = find_empty_entry(root);
    if(entry == -1){
      std::cout << "There is no room for a dir!" << std::endl;
      free(root);
      return 0;
    }
    //find free block in FAT
    int new_block = find_empty_block();
    if(new_block == -1){
      std::cout << "There is no room in FAT!" << std::endl;
      free(root);
      return 0;
    }
    //make new dir
    dir_entry* new_dir = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    for(int i = 0; i < no_dir; i++){
      new_dir[i].size = 0;
      for(int k = 0; k < 56; k++)
        new_dir[i].file_name[k] = '\0';
    }
    //fill first dir pointing to parent
    new_dir[0].file_name[0] = '.';
    new_dir[0].file_name[1] = '.';
    new_dir[0].first_blk = block;
    new_dir[0].type = TYPE_DIR;
    new_dir[0].access_rights = (READ | WRITE | EXECUTE);
    disk.write(new_block, (uint8_t*) new_dir);
    //fill parent dir entry
    for(int i = 0; i < name.length(); i++)
        root[entry].file_name[i] = name[i];
    root[entry].first_blk = new_block;
    root[entry].type = TYPE_DIR;
    root[entry].access_rights = (READ | WRITE | EXECUTE);
    fat[new_block] = FAT_EOF;

    disk.write(block, (uint8_t*) root);
    disk.write(FAT_BLOCK, (uint8_t*) fat);
    free(root);
    free(new_dir);
    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int
FS::cd(std::string dirpath)
{
    disk.read(FAT_BLOCK, (uint8_t*) fat);
    bool check = false;
    int entry = 0;
    //if absolute path
    if(dirpath[0] == '/'){
      current_block = ROOT_BLOCK;
      current_path = "/";
    }
    if(dirpath == "/")
      return 0;
    std::vector<std::string> vec;
    split_text(vec, dirpath);
    //follow path
    while(!vec.empty()){
      root = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
      disk.read(current_block, (uint8_t*) root);
      //check if vec[0] name from dirpath exists
      check = find_file(root, vec[0], entry);
      if(vec[0] == ".."){
        if(current_block == ROOT_BLOCK){
          std::cout << "Can't go back with command (..) in ROOT_BLOCK!" << std::endl;
          free(root);
          return 0;
        }
        //going back a step in current_path
        current_block = root[0].first_blk;
        int last = 0;
        find_last(current_path, last, '/');
        current_path = current_path.erase(last, current_path.length());//changing current_path with a step back
        if(current_path == "")
          current_path = "/";
        vec.erase(vec.begin());
      }
      else{
        if(check){//if entry found
          if(root[entry].type == TYPE_FILE){
            std::cout << "Can't use command (cd) to a file!" << std::endl;
            free(root);
            return 0;
          }
          //go forward to dir with name vec[0]
          current_block = root[entry].first_blk;
          current_path += '/';
          current_path.append(vec[0]);
          vec.erase(vec.begin());
        }
        else{
          std::cout << "Can't find dir: " << vec[0] << std::endl;
          free(root);
          return 0;
        }
      }
      free(root);
    }
    //can happen that current_path is //name
    //this will make //name -> /name
    if(current_path[1] == '/')
      current_path = current_path.substr(1, current_path.length());
    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int
FS::pwd()
{
    std::cout << current_path << std::endl;
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int
FS::chmod(std::string accessrights, std::string filepath)
{
    root = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    int entry, block, check, type;
    check = follow_path(filepath, block, entry, type);

    disk.read(block, (uint8_t*) root);
    disk.read(FAT_BLOCK, (uint8_t*) fat);
    //if path could not be followed
    if(check == -1){
      std::cout << "Can't follow path!" << std::endl;
      free(root);
      return 0;
    }
    //set access rights
    if(accessrights == "1")
      root[entry].access_rights = (EXECUTE);
    else if(accessrights == "2")
      root[entry].access_rights = (WRITE);
    else if(accessrights == "3")
      root[entry].access_rights = (WRITE | EXECUTE);
    else if(accessrights == "4")
      root[entry].access_rights = (READ);
    else if(accessrights == "5")
      root[entry].access_rights = (READ | EXECUTE);
    else if(accessrights == "6")
      root[entry].access_rights = (READ | WRITE);
    else if(accessrights == "7")
      root[entry].access_rights = (READ | WRITE | EXECUTE);
    else
      std::cout << "Can't set accessrights!" << std::endl;
    
    disk.write(current_block, (uint8_t*) root);
    free(root);
    return 0;
}
//check if filename already exists in dir and where
bool
FS::find_file(const dir_entry* point, const std::string &path, int &entry)
{
    bool check = false;
    std::string temp;
    entry = -1;
    for(int i = 0; i < no_dir; i++){
      temp = point[i].file_name;
      if(path == temp){
        check = true;//found file/dir
        entry = i;//found file/dir on index i in given dir
        break;
      }
    }
    return check;
}
int
FS::find_empty_entry(const dir_entry* point)//find empty entry in given entry
{
  int entry = -1;
  for(int i = 0; i < no_dir; i++){
    if(point[i].file_name[0] == '\0'){
      entry = i;
      break;
    }
  }
  return entry;
}
int
FS::find_empty_block()//find empty block in FAT
{
  disk.read(FAT_BLOCK, (uint8_t*) fat);
  int block = -1;
  for(int i = 2; i < BLOCK_SIZE/2; i++){
    if(fat[i] == FAT_FREE){
      block = i;
      break;
    }
  }
  return block;
}
void
FS::find_last(std::string &path, int &index, char chr)//find last chr in path
{
  index = 0;
  for(int i = 0; i < path.length(); i++)
    if(path[i] == chr)
      index = i;
}
void
FS::split_text(std::vector<std::string> &vec, const std::string &path)
{
  std::string temp;
  for(int i = 0; i < path.length(); i++){
    if(path[i] != '/'){
      temp += path[i];
    }
    else{
      if(temp != ""){//add to vec and reset temp
        vec.push_back(temp);
        temp = "";
      }
    }
  }
  if(temp != "")
    vec.push_back(temp);//add last name to vec
}
int
FS::follow_path(std::string &path, int &block, int &entry, int &type)//follow path and get in which block dir/file is in and what entry spot file is in and what type
{
  int this_block = -1;
  std::vector<std::string> vec;
  split_text(vec, path);//split the path so it is easier to follow
  if(path[0] == '/')//absolute path
    this_block = ROOT_BLOCK;
  else//relative path
    this_block = current_block;
  block = this_block;
  type = TYPE_DIR;
  //follow path
  dir_entry* dir;
  bool check = false;
  disk.read(FAT_BLOCK, (uint8_t*) fat);
  while(!vec.empty()){
    dir = (dir_entry*)malloc(sizeof(dir_entry)*no_dir);
    disk.read(this_block, (uint8_t*) dir);

    check = find_file(dir, vec[0], entry);//find on what index the entry for vec[0] is and check if the entry exist
    if(vec[0] == ".."){
      if(this_block != ROOT_BLOCK){//go back a step
        this_block = dir[0].first_blk;
        block = this_block;
      }
      else{//cant go back in root
        free(dir);
        return -1;
      }
    }
    else if(check == true){//name exists in current dir
      if(dir[entry].type == TYPE_FILE && vec.size() == 1){//exist and is file, will still be previous block for the dir the file is in
        type = TYPE_FILE;
        free(dir);
        return 0;
      }
      else if(dir[entry].type == TYPE_DIR){//is a dir and will change block to new dir block
        this_block = dir[entry].first_blk;
        block = this_block;
      }
      else{//file in the middle of the path
        free(dir);
        return -1;
      }
    }
    else if(check == false && vec.size() == 1){//did not find the last name which is what we want in some functions
      type = TYPE_FILE;                        //that is why type = TYPE_FILE now so there is something that tells the code that this is the case
      free(dir);
      return -1;
    }
    else if(check == false){//did not find the entry and is not the last in the path
      free(dir);
      return -1;
    }
    vec.erase(vec.begin());//erase the first name so the path can be followed correctly
    free(dir);
  }
  return 0;
}
bool
FS::req_read(int req)//check if READ is in access_rights
{
  bool check = false;
  if(req == 4 || req == 5 || req == 6 || req == 7)
    check = true;
  return check;
}
bool
FS::req_write(int req)//check if WRITE is in access_rights
{
  bool check = false;
  if(req == 2 || req == 3 || req == 6 || req == 7)
    check = true;
  return check;
}
void
FS::trim(std::string &path)//Delete all extra '/' right to the path, /name/// -> /name
{
  bool loop = true;
  while(loop){
    if(path[path.length()] == '/')
      path = path.substr(0, path.length()-1);
    else
      loop = false;
  }
}
