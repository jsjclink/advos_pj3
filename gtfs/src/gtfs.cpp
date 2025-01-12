#include "gtfs.hpp"
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <sstream>
#include <unistd.h>

#define VERBOSE_PRINT(verbose, str...) do { \
    if (verbose) cout << "VERBOSE: "<< __FILE__ << ":" << __LINE__ << " " << __func__ << "(): " << str; \
} while(0)

int do_verbose;
vector<gtfs_t *> efd;

string generate_unique_id() {
    static int unique_id_counter = 0;
    int pid = getpid();
    
    std::stringstream ss;
    ss << pid << "_" << unique_id_counter++;
    return ss.str();
}

int find_write(file_t* fl,string id){
    int ret = 0;
    for (const auto& write: fl->writes) {
         if(write->id == id) ret = 1;
    }
    return ret;
}

int trct_disk_log(file_t* file){
    int ret = -1;
    int out = 1;
    char line[256];

    while (out) {
        write_t *write_id = new (std::nothrow) write_t();
        for (int i = 0; i < 5; ++i) {
            if (fgets(line, sizeof(line), file->log) != nullptr) {
                string strl = line;
                size_t colonPos = strl.find(':');
                if(i==0 ){
                    write_id->id = strl.substr(colonPos+1);
                    if(!find_write(file,write_id->id)) continue;
                }
                else if(i==1){
                    write_id->filename = strl.substr(colonPos+1);
                }
                else if(i==2){
                    write_id->offset = stoi(strl.substr(colonPos+1));
                }
                else if(i==3){
                    write_id->length = stoi(strl.substr(colonPos+1));
                }
                else{
                    write_id->data = new char[write_id->length];
                    memcpy(write_id->data, strl.substr(colonPos+1).c_str(), write_id->length);
                }
            } else {
                out = 0;
                break;
            }
        }
        if(out){
            write_id->filep = file;
            write_id->com = 1;
            file->writes.push_back(write_id);
        }
    }
    ret = 0;
    return ret;
}


int trct_mem_log(file_t* file){
    int ret = -1;
    for (const auto& write: file->writes){
        if (fseek((write->filep)->fp, write->offset, SEEK_SET) != 0) {
            VERBOSE_PRINT(do_verbose, "Seek(moving to offset) failed\n");
            return ret;
        }
        size_t data_len = fwrite(write->data,sizeof(char),write->length,(write->filep)->fp);
        if(data_len != write->length){
            VERBOSE_PRINT(do_verbose, "Write failed\n");
            return ret;
        }
        delete write->data;
        delete write;
    }
    file->writes.clear();
    fclose(file->log);
    file->log = fopen((file->filename + ".log").c_str(),"w+");
    return 0;
}


gtfs_t* gtfs_init(string directory, int verbose_flag) {
    do_verbose = verbose_flag;
    gtfs_t *gtfs = NULL;
    int found = 0;
    VERBOSE_PRINT(do_verbose, "Initializing GTFileSystem inside directory " << directory << "\n");

    for (const auto& dir: efd) {
        if(dir->dirname == directory) {
            gtfs = dir;
            found = 1;
            break;
        }
    }
    //Directory doesn't exist
    if(!found){
        gtfs = new (std::nothrow) gtfs_t();
        if(!gtfs){
            VERBOSE_PRINT(do_verbose, "FAIL:malloc error\n");
            return NULL;
        }
        gtfs->dirname = directory;
        efd.push_back(gtfs);
    }
    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns non NULL.
    return gtfs;
}

int gtfs_clean(gtfs_t *gtfs) {
    int ret = -1;
    if (gtfs) {
        VERBOSE_PRINT(do_verbose, "Cleaning up GTFileSystem inside directory " << gtfs->dirname << "\n");
        for (const auto& file: gtfs->fsq){
            if(trct_disk_log(file) <0 ){
                VERBOSE_PRINT(do_verbose, "Error while truncating log\n");
                return ret;
            }
            if(trct_mem_log(file) <0 ){
                VERBOSE_PRINT(do_verbose, "Error while truncating log\n");
                return ret;
            }
        }
        
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem does not exist\n");
        return ret;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns 0.
    return ret;
}

file_t* gtfs_open_file(gtfs_t* gtfs, string filename, int file_length) {
    file_t *fl = NULL;
    int found = 0;
    fstream file;
    if (!gtfs) {
        VERBOSE_PRINT(do_verbose, "GTFileSystem does not exist\n");
        return NULL;
    }

    VERBOSE_PRINT(do_verbose, "Opening file " << filename << " inside directory " << gtfs->dirname << "\n");
    //If file length is too long
    if(filename.length() > MAX_FILENAME_LEN){
        VERBOSE_PRINT(do_verbose, "Filename too long\n");
        return NULL;
    }
    //find file inside dir
    vector<file_t *>::iterator itr;
    for (itr = gtfs->fsq.begin(); itr != gtfs->fsq.end(); ++itr) {
        if((*itr)->filename == filename) {
            fl = *itr;
            found = 1;
            break;
        }
    }
    //file exists return file
    if(found){
        if(fl->file_length < file_length){
            fl->file_length = file_length;
        }
        else{
            VERBOSE_PRINT(do_verbose, "File Size is Larger then File Length!\n");
            return NULL;
        }
    }
    //file doesn't exist, return new file
    else{
        if(gtfs->fsq.size() == MAX_NUM_FILES_PER_DIR){
            VERBOSE_PRINT(do_verbose, "Directory Full\n");
            return NULL;
        }
        fl = new (std::nothrow) file_t();
        if(!fl){
            VERBOSE_PRINT(do_verbose, "Malloc Failed\n");
            return NULL;
        }
        fl->file_length = file_length;
        fl->filename = filename;
        fl->fp = fopen(filename.c_str(), "r+");
        fl->log = fopen((filename + ".log").c_str(),"r+");
        //if file doesn't exist in disk, create new file and corresponding log file.
        if(fl->log){
            trct_disk_log(fl);
            trct_mem_log(fl);
        }
        if(!fl->fp) {
            fl->fp = fopen(filename.c_str(),"w+");
            if(!fl->fp){
                VERBOSE_PRINT(do_verbose, "File Open Failed!\n");
                delete fl;
                return NULL;
            }
            //create log file
            fl->log = fopen((filename + ".log").c_str(),"w+");
        }
        
        gtfs->fsq.push_back(fl);
    }

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns non NULL.
    return fl;
}

int gtfs_close_file(gtfs_t* gtfs, file_t* fl) {
    int ret = -1;
    int found = 0;

    if(!(gtfs and fl)) {
        VERBOSE_PRINT(do_verbose, "GTFileSystem or file does not exist\n");
        return ret;
    }

    VERBOSE_PRINT(do_verbose, "Closing file " << fl->filename << " inside directory " << gtfs->dirname << "\n");
    vector<file_t *>::iterator itr;
    for (itr = gtfs->fsq.begin(); itr != gtfs->fsq.end(); ++itr) {
        if((*itr)->filename == fl->filename) {
            fl = *itr;
            found = 1;
            gtfs->fsq.erase(itr);
            break;
        }
    }
    if(found){
        if(trct_disk_log(fl) <0 ){
                VERBOSE_PRINT(do_verbose, "Error while truncating log\n");
                return ret;
            }
        if(trct_mem_log(fl) <0 ){
            VERBOSE_PRINT(do_verbose, "Error while truncating log\n");
            return ret;
        }
        if(fclose(fl->fp)){
            VERBOSE_PRINT(do_verbose, "File Close Error\n");
            return ret;
        }
        fl->fp = NULL;
    }
    else{
        VERBOSE_PRINT(do_verbose, "File Not in Directory\n");
        return ret;
    }
    // free fl

    delete fl;

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns 0.
    ret = 0;
    return ret;
}

int gtfs_remove_file(gtfs_t* gtfs, file_t* fl) {
    int ret = -1;
    int found = 0;

    if (!(gtfs and fl)) {
        VERBOSE_PRINT(do_verbose, "GTFileSystem or file does not exist\n");
        return ret;
    }

    VERBOSE_PRINT(do_verbose, "Removing file " << fl->filename << " inside directory " << gtfs->dirname << "\n");
    vector<file_t *>::iterator itr;
    for (itr = gtfs->fsq.begin(); itr != gtfs->fsq.end(); ++itr) {
        if((*itr)->filename == fl->filename) {
            fl = *itr;
            found = 1;
            break;
        }
    }
    if(found){
        if(!fl->fp){
            VERBOSE_PRINT(do_verbose, "File is still Open\n");
            return ret;
        }

        // todo: remove file from disk, remove log file

        gtfs->fsq.erase(itr);
        if(fclose(fl->log)){
            VERBOSE_PRINT(do_verbose, "File Close Error\n");
            return ret;
        }
        delete fl;
    }
    else{
        VERBOSE_PRINT(do_verbose, "File Not in Directory\n");
        return ret;
    }

    ret = 0;
    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns 0.
    return ret;
}

char* gtfs_read_file(gtfs_t* gtfs, file_t* fl, int offset, int length) {
    char* ret_data = new char[length];
    int cur_pid = getpid();
    size_t pos;
    int pid;
    memset(ret_data, 0, length);
    if(!(gtfs and fl && fl->fp)) {
        VERBOSE_PRINT(do_verbose, "GTFileSystem or file or fp does not exist\n");
        return NULL;
    }

    VERBOSE_PRINT(do_verbose, "Reading " << length << " bytes starting from offset " << offset << " inside file " << fl->filename << "\n");
    fseek(fl->fp, offset, SEEK_SET);
    fread(ret_data, sizeof(char), length, fl->fp);
    // read current writes in memory
    for (const auto& write: fl->writes) {
        pos = write->id.find('_');
        pid = stoi(write->id.substr(0, pos));
        if(pid != cur_pid) {
            if(!(write->com)) continue;
        }
        if (write->offset < offset + length and write->offset + write->length > offset) {
            int write_start = std::max(offset, write->offset);
            int write_end = std::min(offset + length, write->offset + write->length);
            int write_length = write_end - write_start;

            memcpy(ret_data + (write_start - offset), write->data + (write_start - write->offset), write_length);
        }
    }
    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns pointer to data read.
    return ret_data;
}

write_t* gtfs_write_file(gtfs_t* gtfs, file_t* fl, int offset, int length, const char* data) {
    write_t *write_id = NULL;

    if (!(gtfs and fl)) {
        VERBOSE_PRINT(do_verbose, "GTFileSystem or file does not exist\n");
        return NULL;
    }

    if (offset + length > fl->file_length) {
        VERBOSE_PRINT(do_verbose, "Write exceeds file length\n");
        return NULL;
    }

    VERBOSE_PRINT(do_verbose, "Writting " << length << " bytes starting from offset " << offset << " inside file " << fl->filename << "\n");
    
    write_id = new (std::nothrow) write_t();
    if(!write_id){
        VERBOSE_PRINT(do_verbose, "Malloc Failed\n");
        return NULL;
    }

    write_id->data = new char[length];
    memcpy(write_id->data, data, length);
    write_id->offset = offset;
    write_id->length = length;
    write_id->filep = fl;
    write_id->filename = fl->filename;
    write_id->id = generate_unique_id();
    write_id->com = 0;
   // write_id->log = fl->log;
    
    // string logstr = "0" + to_string(length) + " " + to_string(offset) + " " + data;

    // size_t data_len = fwrite(&logstr,sizeof(char),logstr.length(),fl->log);
    // if(data_len != logstr.length() ){
    //     VERBOSE_PRINT(do_verbose, "Write to log Failed\n");
    //     delete write_id;
    //     return NULL;
    // }

    fl->writes.push_back(write_id);


    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns non NULL.
    return write_id;
}

int gtfs_sync_write_file(write_t* write_id) {
    int ret = -1;

    if(!(write_id and write_id->filep and (write_id->filep)->log)) {
        VERBOSE_PRINT(do_verbose, "Write operation does not exist\n");
        return ret;
    }

    VERBOSE_PRINT(do_verbose, "Persisting write of " << write_id->length << " bytes starting from offset " << write_id->offset << " inside file " << write_id->filename << "\n");
    /*
    // write data to file
    if (fseek((write_id->filep)->fp, write_id->offset, SEEK_SET) != 0) {
        VERBOSE_PRINT(do_verbose, "Seek(moving to offset) failed\n");
        return ret;
    }
    size_t data_len = fwrite(write_id->data,sizeof(char),write_id->length,(write_id->filep)->fp);
    if(data_len == write_id->length) ret = data_len;
    else{
        VERBOSE_PRINT(do_verbose, "Write failed\n");
        return ret;
    }
    */
    // write log file
    int log_result = fprintf((write_id->filep)->log, "write_id:%s\nfilename:%s\noffset:%d\nlength:%d\ndata:%.*s\n\n",
                             write_id->id.c_str(), write_id->filename.c_str(), write_id->offset, write_id->length, write_id->length, write_id->data);
    if (log_result < 0) {
        VERBOSE_PRINT(do_verbose, "Write to log failed\n");
        return ret;
    }
    else{
        ret = write_id->length;
    }
    fflush((write_id->filep)->log);
    write_id->com = 1;
    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns number of bytes written.
    return ret;
}

int gtfs_abort_write_file(write_t* write_id) {
    int ret = -1;
    int count = 0;
    int found = 0;
    if (write_id) {
        VERBOSE_PRINT(do_verbose, "Aborting write of " << write_id->length << " bytes starting from offset " << write_id->offset << " inside file " << write_id->filename << "\n");
        for (const auto& write: write_id->filep->writes) {
            if(write->id == write_id->id){
                write_id->filep->writes.erase(write_id->filep->writes.begin()+count);
                found = 1;
                break;
            }
            count ++;
        }
        if(!found){
            VERBOSE_PRINT(do_verbose, "Write operation does not exist\n");
            return ret;
        }
        delete write_id->data;
        delete write_id;
    } else {
        VERBOSE_PRINT(do_verbose, "Write operation does not exist\n");
        return ret;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality
    ret = 0;
    VERBOSE_PRINT(do_verbose, "Success.\n"); //On success returns 0.
    return ret;
}

// BONUS: Implement below API calls to get bonus credits

int gtfs_clean_n_bytes(gtfs_t *gtfs, int bytes){
    int ret = -1;
    if (gtfs) {
        VERBOSE_PRINT(do_verbose, "Cleaning up [ " << bytes << " bytes ] GTFileSystem inside directory " << gtfs->dirname << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem does not exist\n");
        return ret;
    }

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns 0.
    return ret;
}

int gtfs_sync_write_file_n_bytes(write_t* write_id, int bytes){
    int ret = -1;
    if (write_id) {
        VERBOSE_PRINT(do_verbose, "Persisting [ " << bytes << " bytes ] write of " << write_id->length << " bytes starting from offset " << write_id->offset << " inside file " << write_id->filename << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "Write operation does not exist\n");
        return ret;
    }
    char *data = new char[write_id->length];
    memcpy(data, write_id->data, bytes);
    // write log file
    int log_result = fprintf((write_id->filep)->log, "write_id: %s\nfilename: %s\noffset: %d\nlength: %d\ndata: %.*s\n\n",
                             write_id->id.c_str(), write_id->filename.c_str(), write_id->offset, write_id->length, write_id->length, data);
    if (log_result < 0) {
        VERBOSE_PRINT(do_verbose, "Write to log failed\n");
        return ret;
    }
    else{
        ret = bytes;
    }
    fflush((write_id->filep)->log);
    write_id->com = 1;
    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns 0.
    return ret;
}

