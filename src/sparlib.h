#ifndef SPARLIB_H
#define SPARLIB_H
/*
 * sparlib
 * 
 * write/read parallel archive
 *  
 * (c) Holger Berger 2018
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>

#include "multistreamfile.h"

namespace sparlib {

	// definition of file format

	// spar
	const uint32_t sectionmagic = 0x70737261;

	// header in front of any section
	struct SectionHeader {
	    int32_t magic;  // spar
	    int16_t type;   // SectionType
	    int16_t size;   // size of header data (not including this header) in bytes
	};

	// possible sections
	enum SectionType {
	    FileHeader,     // start of a file
	    DirHeader,      // a directory
	    SoftLinkheader, // a link
	    FileBody,       // part of a file body 
	    FileFooter,     // end of a file
	    StreamEnd       // mark end of stream
	};


	// possible compression within a FileBody Section
	enum CompressonType {
	    NoneC,
	    SnappyC,
	    ZstdC
	};

	// possible checksums for a file
	enum HashType {
	    NoneH,
	    CRC64H,
	    Sha256H
	};

	// header of a file, to be marshalled 
	struct FileHeader {
	    uint64_t fileid;// id, unique within the archive file
	    uint64_t size;  // size of file
	    uint64_t mode;  // file mode
	    uint64_t ctime; // future use
	    uint64_t mtime; // future use
	    uint64_t atime; // future use
	    uint32_t uid;   // future use
	    uint32_t gid;   // future use
	    //std::string owner;   // future use
	    //std::string group;   // future use
	    uint16_t hash;       // HashType
	    // std::string name; // in file, this comes after the struct and fills the remaining space!!!
	};

	// part of body of a file, each segment can be compressed with different method
	// for adaptive compression
	struct FileBody {
	    uint64_t fileid;        // file this belongs to
	    uint64_t size;          // size of the fragment (after compression)
	    uint16_t compression;   // type of compression, CompressionType
	};

	// end if a file, fileid can be freed, and file can be closed
	// in extraction
	struct FileFooter {
	    uint64_t fileid;
	    uint64_t crc1;  // space for crc64
	    uint64_t crc2;  // room for more bits in case of sha256
	    uint64_t crc3;
	    uint64_t crc4;
	};

	/// END of file format definition

	struct FileEntry {
	    std::string path, filename;
	    struct stat fileinfo;
	};

	class Writer {
	public:
	    Writer(multistreamfile::MultistreamFile* multistreamfile, int threads);

	    void addFile(const std::string path, const std::string filename, const struct stat fileinfo);
	    void addDirectory(const std::string path, const std::string filename, const struct stat fileinfo);
	    void close();
	    
	    uint64_t writeFileHeader(std::string path, std::string filename, struct stat fileinfo, int stream);
	    void writeFileSegment(uint64_t fileid, char* buffer, size_t size, int stream);
	    void writeFileFooter(uint64_t fileid, int stream);

	private:
	    void static fileWorker(sparlib::Writer* t, int id);

	    multistreamfile::MultistreamFile* multistreamfile;
	    int threads;
	    std::vector<std::thread*> threadlist;
	    std::deque<FileEntry> filequeue;
	    std::deque<FileEntry> dirqueue;
	    std::mutex fqmutex;
	    std::mutex dqmutex;
	    std::condition_variable fqcond;
	    std::mutex stdiomutex;
	    size_t bytes_done;
	    std::chrono::time_point<std::chrono::high_resolution_clock> real_time_s, real_time_e;
	    std::chrono::duration<double> real_time;
	    uint64_t nextfileid;
	    std::mutex fileidmutex;
	};

}

#endif
