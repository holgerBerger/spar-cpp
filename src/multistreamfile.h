#ifndef MULTISTREAMFILE_H
#define MULTISTREAMFILE_H

/*
 * spar multistreamfile
 * 
 * abstraction for a file containing several parallel streams
 * 
 * (c) Holger Berger 2018
 */

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <cinttypes>

namespace multistreamfile {

    enum MultistreamFilemode { read, write };

    const uint64_t filemagic = 0x5241505352415053L; // SPARSPAR
    const uint16_t fileversion = 1; // VERSION


    // this if first bytes in a file
    struct FileHeader {
        const uint64_t magic = filemagic;
        const uint16_t version = fileversion;
        uint16_t streams;
        uint32_t buffersize;
        uint64_t ctime;
    };

    struct Buffer {
        ssize_t bpos; 	// offset for next write
        int filepos;	// blockid to write next
        char* buffer; // start of buffer
        size_t bytes_done; // counter for volume
        std::mutex* mutex;  	// mutex to protect access on this buffer
    };


    class MultistreamFile {
    public:
        // open for writing
        MultistreamFile(const std::string filename, const size_t buffersize, const int streams);
        // open for reading
        MultistreamFile(const std::string filename);

        ~MultistreamFile();

        // write a block into a stream
        ssize_t write(const char *data, const ssize_t bytes, const int streamid);
        ssize_t read (char *buffer, const ssize_t bytes, const int streamid);
        
        void lock(int streamid);
        void unlock(int streamid);

        // get buffersize this should be written with
        ssize_t getBuffersize() {return buffersize;};
        int getStreams() {return streams;};

    private:
        MultistreamFilemode mode; 			// read or write
        std::string filename;
        ssize_t buffersize;				// size of buffer, also used for alignement
        int streams;					// #streams
        int fh;						// file handle
        std::vector<Buffer> buffers;			// list of buffers
        // close file after flushing all stream buffers
        void flush();
    };
    }
#endif
