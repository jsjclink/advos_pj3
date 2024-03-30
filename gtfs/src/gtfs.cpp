#include "gtfs.hpp"

#define VERBOSE_PRINT(verbose, str...) do { \
    if (verbose) cout << "VERBOSE: "<< __FILE__ << ":" << __LINE__ << " " << __func__ << "(): " << str; \
} while(0)

int do_verbose;
vector<gtfs_t *> efd;


//todo!!
// fread보다는 fseek와 ftell로 파일 크기를 읽는게 바람직함
// 파일을 해당 크기로 생성할 때는 그냥 0으로 채우는게 나을 듯
// 따라서 fl->data는 file content를 의미하는게 아닌 것 같음

gtfs_t* gtfs_init(string directory, int verbose_flag) {
    do_verbose = verbose_flag;
    gtfs_t *gtfs = NULL;
    int found = 0;
    VERBOSE_PRINT(do_verbose, "Initializing GTFileSystem inside directory " << directory << "\n");

    vector<gtfs_t *>::iterator itr;
    for (itr = efd.begin(); itr != efd.end(); ++itr) {
        if((*itr)->dirname == directory) {
            gtfs = *itr;
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
    if (gtfs) {
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
                // realloc이 아니라 file 크기 자체를 바꾸는 것 같음
                fl->data = realloc(fl->data,file_length);
                if(fread(fl->data,sizeof(char),file_length,fl->fp) != file_length){
                    VERBOSE_PRINT(do_verbose, "File Read Failed!\n");
                    return NULL;
                }
            }
            else if(fl->file_length == file_length){
                // file 내용을 읽어올 필요가 없는 것 같음
                if(fread(fl->data,sizeof(char),file_length,fl->fp) != file_length){
                    VERBOSE_PRINT(do_verbose, "File Read Failed!\n");
                    return NULL;
                }
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
            // file content를 여기에 저장하는게 아닌듯?? 추가 data 저장할 게 있으면 하는거같음
            fl->data = malloc(file_length);
            if(!fl->data){
                VERBOSE_PRINT(do_verbose, "Malloc Failed\n");
                delete fl;
                return NULL;
            }
            fl->file_length = file_length;
            fl->filename = filename;
            fl->fp = fopen(filename.c_str(), "r+");
            if(fl->fp) {
                fread(fl->data,sizeof(char),file_length,fl->fp);
            } else {
                fl->fp = fopen(filename.c_str(),"w");
                if(!fl->fp){
                    VERBOSE_PRINT(do_verbose, "File Open Failed!\n");
                    free(fl->data);
                    delete fl;
                    return NULL;
                }
            }
            
            gtfs->fsq.push_back(fl);
        }
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem does not exist\n");
        return NULL;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns non NULL.
    return fl;
}

int gtfs_close_file(gtfs_t* gtfs, file_t* fl) {
    int ret = -1;
    int found = 0;
    if (gtfs and fl) {
        VERBOSE_PRINT(do_verbose, "Closing file " << fl->filename << " inside directory " << gtfs->dirname << "\n");
        vector<file_t *>::iterator itr;
        for (itr = gtfs->fsq.begin(); itr != gtfs->fsq.end(); ++itr) {
            if((*itr)->filename == fl->filename) {
                fl = *itr;
                found = 1;
                break;
            }
        }
        if(found){
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
        
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem or file does not exist\n");
        return ret;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns 0.
    ret = 0;
    return ret;
}

int gtfs_remove_file(gtfs_t* gtfs, file_t* fl) {
    int ret = -1;
    int found = 0;
    if (gtfs and fl) {
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
            gtfs->fsq.erase(itr);
            free(fl->data);
            delete fl;
        }
        else{
            VERBOSE_PRINT(do_verbose, "File Not in Directory\n");
            return ret;
        }
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem or file does not exist\n");
        return ret;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality
    ret = 0;
    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns 0.
    return ret;
}

char* gtfs_read_file(gtfs_t* gtfs, file_t* fl, int offset, int length) {
    char* ret_data = NULL;
    if (gtfs and fl) {
        VERBOSE_PRINT(do_verbose, "Reading " << length << " bytes starting from offset " << offset << " inside file " << fl->filename << "\n");
        //TODO: This is for regular files, should change to actual write versions.
        if (fseek(fl->fp, offset, SEEK_SET) != 0) {
            VERBOSE_PRINT(do_verbose, "Seek(moving to offset) failed\n");
            return NULL;
        }
        ret_data = new char[length];
        size_t data_len = fread(ret_data,sizeof(char),length,fl->fp);
        cout << data_len;
        cout << ret_data;
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem or file does not exist\n");
        return NULL;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns pointer to data read.
    return ret_data;
}

write_t* gtfs_write_file(gtfs_t* gtfs, file_t* fl, int offset, int length, const char* data) {
    write_t *write_id = NULL;
    if (gtfs and fl) {
        VERBOSE_PRINT(do_verbose, "Writting " << length << " bytes starting from offset " << offset << " inside file " << fl->filename << "\n");
        write_id = new (std::nothrow) write_t();
        if(!write_id){
            VERBOSE_PRINT(do_verbose, "Malloc Failed\n");
            return NULL;
        }
        write_id->data = const_cast<char*>(data);
        write_id->offset = offset;
        write_id->length = length;
        write_id->filep = fl->fp;
        write_id->filename = fl->filename;
    } else {
        VERBOSE_PRINT(do_verbose, "GTFileSystem or file does not exist\n");
        return NULL;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns non NULL.
    return write_id;
}

int gtfs_sync_write_file(write_t* write_id) {
    int ret = -1;
    if (write_id) {
        VERBOSE_PRINT(do_verbose, "Persisting write of " << write_id->length << " bytes starting from offset " << write_id->offset << " inside file " << write_id->filename << "\n");
        if (fseek(write_id->filep, write_id->offset, SEEK_SET) != 0) {
            VERBOSE_PRINT(do_verbose, "Seek(moving to offset) failed\n");
            return ret;
        }
        size_t data_len = fwrite(write_id->data,sizeof(char),write_id->length,write_id->filep);
        if(data_len == write_id->length) ret = data_len;
        else{
            VERBOSE_PRINT(do_verbose, "Write failed\n");
            return ret;
        }
    } else {
        VERBOSE_PRINT(do_verbose, "Write operation does not exist\n");
        return ret;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns number of bytes written.
    return ret;
}

int gtfs_abort_write_file(write_t* write_id) {
    int ret = -1;
    if (write_id) {
        VERBOSE_PRINT(do_verbose, "Aborting write of " << write_id->length << " bytes starting from offset " << write_id->offset << " inside file " << write_id->filename << "\n");
    } else {
        VERBOSE_PRINT(do_verbose, "Write operation does not exist\n");
        return ret;
    }
    //TODO: Add any additional initializations and checks, and complete the functionality

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

    VERBOSE_PRINT(do_verbose, "Success\n"); //On success returns 0.
    return ret;
}

