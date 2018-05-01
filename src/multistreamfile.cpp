
/*
 * spar multistreamfile
 * 
 * abstraction for a file containing several parallel streams
 * 
 * (c) Holger Berger 2018
 */

#include <exception>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <cinttypes>
#include <algorithm>
#include <cassert>

#include "multistreamfile.h"

extern long FLAGS;

namespace multistreamfile {

    class fileexception : public std::exception {

        virtual const char* what() const throw () {
            return "could not open file";
        }
    } fileexception;

    // open for writing
    MultistreamFile::MultistreamFile(const std::string filename, const size_t buffersize, const int streams)
        : filename(filename), buffersize(buffersize), streams(streams) {
        mode = MultistreamFilemode::write;

        // open file or bail out
        fh = open(filename.c_str(), O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
        if (fh == -1) {
            std::cerr << "can not open file!" << std::endl;
            throw fileexception;
        }

        // allocate buffers and mutexe
        buffers.resize(streams);

        for (int i = 0; i < streams; i++) {
            buffers[i].buffer = new char[buffersize * 2];
            buffers[i].mutex = new std::mutex;
            buffers[i].filepos = 0;
            buffers[i].bpos = 0;
        }

        // write header + infos into streamid0
        auto header = FileHeader();
        header.ctime = time(nullptr);
        header.streams = streams;
        header.buffersize = (uint32_t)buffersize;

        ssize_t ret = write((char *)&header, sizeof(header), 0);
        if (ret != sizeof(header)) {
            std::cerr << "error in writing header!" << std::endl;
        }

    }

    // open for reading

    MultistreamFile::MultistreamFile(const std::string filename)
        : filename(filename) {
        mode = MultistreamFilemode::read;

        fh = open(filename.c_str(), O_RDONLY);
        if (fh<0) {
            // FIXME better error handling
            std::cerr << "can not open file!" << std::endl;
            throw fileexception;
        }

        struct FileHeader fheader;
        ssize_t ret = ::read(fh, &fheader, sizeof(fheader));

        if (fheader.magic!=filemagic) {
            std::cerr << "this is not a spar multistream file!" << std::endl;
            throw fileexception;
        }

        if (fheader.version>fileversion) {
            std::cerr << "this file has a newer version than this tool!" << std::endl;
        }

        streams = fheader.streams;
        buffersize = fheader.buffersize;

        // std::cout << "streams: " << fheader.streams << std::endl;
        // std::cout << "buffersize: " << fheader.buffersize << std::endl;

        // allocate buffers and mutexe
        buffers.resize(streams);

        for (int i = 0; i < streams; i++) {
            buffers[i].buffer = new char[buffersize * 2];
            buffers[i].mutex = new std::mutex;
            buffers[i].filepos = 0;
            buffers[i].bpos = 0;
        }

        // read again into buffers
        ret = read((char *)&fheader, sizeof(fheader), 0);
    }


    MultistreamFile::~MultistreamFile() {

        if (mode==MultistreamFilemode::write) {
            // fist make sure all data is in file
            flush();
        }

        // get all locks before closing file
        for (int i = 0; i < streams; i++) {
            buffers[i].mutex->lock();
        }

        // close file
        int ret = close(fh);
        if (ret == -1) {
            std::cerr << "error in close" << errno << std::endl;
        }


        // free resources
        for (int i = 0; i < streams; i++) {
            if(FLAGS&1) {
                std::cout << "MultistreamFile stream " << i << ": " << buffers[i].bytes_done/(1024*1024) << " MB" << std::endl;
            }
            delete[] buffers[i].buffer;
            delete buffers[i].mutex;
        }

    }


    void MultistreamFile::lock(int streamid)
    {
        buffers[streamid].mutex->lock();
    }

    void MultistreamFile::unlock(int streamid)
    {
        buffers[streamid].mutex->unlock();
    }



    // write <bytes> bytes, can be larger than <blocksize>, but not larger than blocksize*2
    // backend will always see at most blocksize IOs aligned to blocksize
    ssize_t MultistreamFile::write(const char *data, const ssize_t bytes, const int streamid) {
        if (bytes > buffersize * 2) return -1;
        if (streamid >= streams) return -2;

        memcpy(buffers[streamid].buffer + buffers[streamid].bpos, data, bytes);
        buffers[streamid].bpos += bytes;
        if (buffers[streamid].bpos >= buffersize) {
            ssize_t ret;
            // std::cout << "pwrite (" <<  streamid << ") to " << (buffers[streamid].filepos*streams+streamid)*buffersize << std::endl;
            if (streams > 1) {
                ret = pwrite(fh, buffers[streamid].buffer, buffersize, 
                    (buffers[streamid].filepos*streams+streamid)*buffersize);
            } else {
                ret = ::write(fh, buffers[streamid].buffer, buffersize);
            }
         
                    
            // FIXME check return value and repeat until done
            if (ret != buffersize) {
                std::cerr << "pwrite failed!" << ret << std::endl;
                exit(-1);
            }
            buffers[streamid].filepos++;

            assert(buffers[streamid].bpos - buffersize >= 0);
            memmove(buffers[streamid].buffer, buffers[streamid].buffer + buffersize,
                    buffers[streamid].bpos - buffersize);

            buffers[streamid].bpos -= buffersize;
            buffers[streamid].bytes_done += buffersize;
        }

        return bytes;
    }

    // read from stream
    ssize_t MultistreamFile::read(char *buffer, const ssize_t bytes, const int streamid) {
        ssize_t ret;
        if (bytes>buffersize *2) return -1;
        if (streamid >= streams) return -2;
        
        if (buffers[streamid].bpos < bytes) {
            ret = pread(fh, buffers[streamid].buffer + buffers[streamid].bpos,
                        buffersize, (buffers[streamid].filepos*streams+streamid)*buffersize);
            if (ret == -1) {
                std::cerr << "pread failed!" << ret << std::endl;
                exit(-1);
            }
            if(ret>0) buffers[streamid].filepos++;
            buffers[streamid].bpos += ret;
        }
        ssize_t minlen = std::min(bytes, buffers[streamid].bpos);
        // copy to user
        memcpy(buffer, buffers[streamid].buffer , minlen );
        // shift in buffer
        memmove(buffers[streamid].buffer, buffers[streamid].buffer+std::min(bytes,buffers[streamid].bpos), buffers[streamid].bpos-minlen );
        buffers[streamid].bpos -= minlen;
        buffers[streamid].bytes_done += minlen;

        return std::min(bytes, minlen);
    }

    // flush the stream, call this only before closing the file !!!!!
    // calling multiple times leads to errors!
    void MultistreamFile::flush() {
        for (int streamid = 0; streamid < streams; streamid++) {
            
            if (buffers[streamid].bpos > 0) {
                
                // std::cout << "flush pwrite (" <<  streamid << ") to " << (buffers[streamid].filepos*streams+streamid)*buffersize <<
                // ", " << buffers[streamid].bpos << " bytes " << buffers[streamid].filepos  << std::endl;
                ssize_t ret;
                if (streams>1) {
                    ret = pwrite(fh, buffers[streamid].buffer, buffers[streamid].bpos, (buffers[streamid].filepos*streams + streamid) * buffersize);
                } else {
                    ret = ::write(fh, buffers[streamid].buffer, buffers[streamid].bpos);
                }
                //FIXME check return value and repeat until done
                if (ret != buffers[streamid].bpos) {
                    std::cerr << "pwrite failed!" << ret << std::endl;
                }
                buffers[streamid].bytes_done += buffers[streamid].bpos;
                buffers[streamid].bpos = 0;
            }

        }

    }

}

