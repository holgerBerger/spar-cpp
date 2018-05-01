/*
 * sparlib
 * 
 * write/read parallel archive
 *  
 * TODO
 * code for compression/checksum
 * code for extraction
 * 
 * (c) Holger Berger 2018
 */


#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "sparlib.h"

extern long FLAGS;

namespace sparlib {

    Writer::Writer(multistreamfile::MultistreamFile* multistreamfile, int threads)
        : multistreamfile(multistreamfile), threads(threads) {
        real_time_s = std::chrono::high_resolution_clock::now();
        bytes_done = 0;
        nextfileid=0;
        // spawn workerthreads
        for(int i=0; i<threads; i++) {
            threadlist.push_back(new std::thread(fileWorker, this, i));
        }
    };

    // insert a file into the queue for files to be read and pushed into archive
    void Writer::addFile(const std::string path, const std::string filename, const struct stat fileinfo) {
        std::lock_guard<std::mutex> lock(fqmutex);

        filequeue.push_back({path, filename, fileinfo});
        fqcond.notify_one();
    }

    // signal all threads to finish work and wait for them to finish
    void Writer::close() {
        struct stat fileinfo;
        for(int i=0;  i<threads; i++) {
            addFile("","",fileinfo);
        }

        for(auto &t : threadlist) {
            //std::cout << "waiting " << t->get_id() << std::endl;
            t->join();
        }
        //std::cout << "Writer::close done " << std::endl;
        
        // write end marker into each stream
        int streams = multistreamfile->getStreams();
        for(int stream = 0; stream<streams; stream++) {
            struct SectionHeader sh {
                sectionmagic, StreamEnd, 0L
            };
            multistreamfile->lock(stream);
            multistreamfile->write((char *)&sh, sizeof(sh), stream);
            multistreamfile->unlock(stream);
        }
        
        real_time_e = std::chrono::high_resolution_clock::now();
        real_time = real_time_e - real_time_s;
        if (FLAGS&1) {
            std::cout << "Writer: " << bytes_done/(1024*1024) << " MB, " <<
                      real_time.count() << " sec, " <<
                      bytes_done/real_time.count()/(1024*1024) <<" MB/s" << std::endl;
        }
    }

    // worker thread, waits on filequeue and reads files to send them in multifilestream
    void  Writer::fileWorker(sparlib::Writer* t, int id) {
        long files_done=0, bytes_done=0;
        std::chrono::time_point<std::chrono::high_resolution_clock> real_time_s, real_time_e, io_time_s, io_time_e;
        std::chrono::duration<double> io_time, real_time;

        int streams = t->multistreamfile->getStreams();
        size_t buffersize = t->multistreamfile->getBuffersize();

        real_time_s = std::chrono::high_resolution_clock::now();

        while(true) {
            // get entry from input queue
            std::unique_lock<std::mutex> lock(t->fqmutex);
            t->fqcond.wait(lock, [t] {return !t->filequeue.empty();});
            auto entry = t->filequeue.front();
            t->filequeue.pop_front();
            lock.unlock();

            // exit if empty entry
            if(entry.path=="") break;

            // give others a chance
            std::this_thread::yield();

            // process the file
            io_time_s = std::chrono::high_resolution_clock::now();
            files_done++;

            int fh = open((entry.path+"/"+entry.filename).c_str(), O_RDONLY|O_NOATIME);
            if (fh<0) {
                // FIXME better error handling!
                std::cerr << "error: could not open file " << entry.path + "/" + entry.filename << std::endl;
                continue;
            }

            // WRITE_FILE_HEADER
            uint64_t fileid = t->writeFileHeader(entry.path, entry.filename, entry.fileinfo, id%streams);

            char* buffer = new char[buffersize];

            if (FLAGS&4) std::cout << id << " reading " << entry.filename << " to " << id%streams << " as " << fileid << std::endl;

            while(true) {
                ssize_t size = read(fh, buffer, buffersize);
                if (size <= 0 ) break;
                // WRITE FILE SEGMENT
                t->writeFileSegment(fileid, buffer, size, id%streams);
                bytes_done+=size;
            }

            delete[] buffer;
            ::close(fh);

            // WRITE FILE FOOTER
            t->writeFileFooter(fileid, id%streams);

            io_time_e = std::chrono::high_resolution_clock::now();
            io_time += io_time_e - io_time_s;

            if (FLAGS&8) std::cout << "thread "<< id << " done writing " << entry.filename << std::endl;
        }

        real_time_e = std::chrono::high_resolution_clock::now();
        real_time = real_time_e - real_time_s;

        if (FLAGS&2) {
            std::unique_lock<std::mutex> lock(t->stdiomutex);
            std::cout << "reader " << id << " timing: " << files_done << " files, "
                      << bytes_done/(1024*1024) << " MBytes  "
                      << bytes_done/real_time.count()/(1024*1024) << " MBytes/s (real)  "
                      << bytes_done/io_time.count()/(1024*1024) << " MBytes/s (io)  "
                      << io_time.count() << " secs (io)  "
                      << real_time.count() << " secs"
                      << std::endl;
        }
        std::unique_lock<std::mutex> lock(t->stdiomutex);
        t->bytes_done += bytes_done;
    }


    uint64_t Writer::writeFileHeader(std::string path, std::string filename, struct stat fileinfo, int stream)
    {
        // get fileid and bump it up
        fileidmutex.lock();
        uint64_t myfileid = nextfileid++;
        fileidmutex.unlock();

        // FIXME endianess?

        std::string fullname;

        if (path[path.length()]=='/')
            fullname = path + filename;
        else
            fullname = path + "/" + filename;

        // remove leading / from name
        while (fullname[0]=='/') fullname=fullname.substr(1, std::string::npos);

        // lock stream, as following writes have to be consecutive in file
        multistreamfile->lock(stream);
        
        
        // write section header
        int16_t headersize = (int16_t)(sizeof(struct FileHeader)+fullname.length());
        struct SectionHeader sh {
            sectionmagic, FileHeader, headersize
        };
        
        multistreamfile->write((char*)&sh, sizeof(sh), stream);

        // write file header
        struct FileHeader fh;
        std::memset(&fh, 0, sizeof(fh));
        // FIXME !!!!! fill header!!!!!! 
        
        fh.fileid = myfileid;
        multistreamfile->write((char*)&fh, sizeof(fh), stream);

        // write file path last
        multistreamfile->write((char*)fullname.c_str(), fullname.length(), stream);
        // done

        multistreamfile->unlock(stream);
        
        return myfileid;
    }

    void Writer::writeFileSegment(uint64_t fileid, char* buffer, size_t size, int stream)
    {
        multistreamfile->lock(stream);
        
        // write section header
        struct SectionHeader sh {
            sectionmagic, FileBody, sizeof(struct FileBody)
        };
        multistreamfile->write((char *)&sh, sizeof(sh), stream);

        // FIXME add compression here

        // write body header
        struct FileBody bh {
            fileid, size, NoneC
        };
        multistreamfile->write((char*)&bh, sizeof(bh), stream);
        if (FLAGS&4) std::cout << " writing segment for " << fileid << std::endl;
        // write data
        multistreamfile->write(buffer, size, stream);
        
        multistreamfile->unlock(stream);
    }

    void Writer::writeFileFooter(uint64_t fileid, int stream)
    {
        multistreamfile->lock(stream);
        // write section header
        struct SectionHeader sh {
            sectionmagic, FileFooter, sizeof(struct FileFooter)
        };
        multistreamfile->write((char *)&sh, sizeof(sh), stream);

        // write footer header
        struct FileFooter fh {
            fileid, 0L, 0L, 0L, 0L
        };
        multistreamfile->write((char*)&fh, sizeof(fh),stream);
        
        multistreamfile->unlock(stream);
    }

}
