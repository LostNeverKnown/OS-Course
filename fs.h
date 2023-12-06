#include <iostream>
#include <cstdint>
#include <string>
#include <vector>
#include "disk.h"

#ifndef __FS_H__
#define __FS_H__

#define ROOT_BLOCK 0
#define FAT_BLOCK 1
#define FAT_FREE 0
#define FAT_EOF -1

#define TYPE_FILE 0
#define TYPE_DIR 1
#define READ 0x04
#define WRITE 0x02
#define EXECUTE 0x01

struct dir_entry {
    //file/dir name can be 55 char long, last char has to be /0 to terminate
    char file_name[56]; // name of the file / sub-directory
    uint32_t size; // size of the file in bytes
    uint16_t first_blk; // index in the FAT for the first block of the file
    uint8_t type; // directory (1) or file (0)
    uint8_t access_rights; // read (0x04), write (0x02), execute (0x01)
    //each dir can contain 64 files/dir
};

class FS {
private:
    Disk disk;
    // size of a FAT entry is 2 bytes
    int16_t fat[BLOCK_SIZE/2];
    int no_dir = BLOCK_SIZE/sizeof(dir_entry);
    //dir_entry* root[BLOCK_SIZE/sizeof(dir_entry)];
    dir_entry* root;

public:
    FS();
    ~FS();
    // formats the disk, i.e., creates an empty file system
    int format();
    // create <filepath> creates a new file on the disk, the data content is
    // written on the following rows (ended with an empty row)
    int create(std::string filepath);
    // cat <filepath> reads the content of a file and prints it on the screen
    int cat(std::string filepath);
    // ls lists the content in the current directory (files and sub-directories)
    int ls();

    // cp <sourcepath> <destpath> makes an exact copy of the file
    // <sourcepath> to a new file <destpath>
    int cp(std::string sourcepath, std::string destpath);
    // mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
    // or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
    int mv(std::string sourcepath, std::string destpath);
    // rm <filepath> removes / deletes the file <filepath>
    int rm(std::string filepath);
    // append <filepath1> <filepath2> appends the contents of file <filepath1> to
    // the end of file <filepath2>. The file <filepath1> is unchanged.
    int append(std::string filepath1, std::string filepath2);

    // mkdir <dirpath> creates a new sub-directory with the name <dirpath>
    // in the current directory
    int mkdir(std::string dirpath);
    // cd <dirpath> changes the current (working) directory to the directory named <dirpath>
    int cd(std::string dirpath);
    // pwd prints the full path, i.e., from the root directory, to the current
    // directory, including the current directory name
    int pwd();

    // chmod <accessrights> <filepath> changes the access rights for the
    // file <filepath> to <accessrights>.
    int chmod(std::string accessrights, std::string filepath);
    
    //searches through dir for the path name, give true if it exists and false if not
    bool find_file(const dir_entry* point, const std::string &path, int &entry);
    //find an empty spot for a new entry in dir
    int find_empty_entry(const dir_entry* point);
    //find an empty spot in FAT
    int find_empty_block();
    //find at what index the last chr is in path
    void find_last(std::string &path, int &index, char chr);
    //split the text path with the char '/' as seperator and store all words/names in vec
    void split_text(std::vector<std::string> &vec, const std::string &path);
    //follow path and return 0 if it worked and -1 if not
    //at the same time return the block for the last directory found
    //and what index the last name was found and the type for the last found name
    int follow_path(std::string &path, int &block, int &entry, int &type);
    //find if the directory/file has READ in its access_rights
    bool req_read(int req);
    //find if the directory/file has WRITE in its access_rights
    bool req_write(int req);
    //trim the path so all '/' at the end gets deleted, used so the function find_last works
    void trim(std::string &path);
};

#endif // __FS_H__
