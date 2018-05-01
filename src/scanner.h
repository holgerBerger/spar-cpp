/*
 * spar scanner
 * 
 * parallel file tree scanner
 *  
 * (c) Holger Berger 2018
 */


#ifndef SCANNER_H
#define SCANNER_H
#define BOOST_FILESYSTEM_NO_DEPRECATED

#include <boost/filesystem.hpp>
#include <vector>
#include <initializer_list>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "sparlib.h"

namespace fs = boost::filesystem;



/* small proxy to hold status of a file to avoid repeated stats,
   does one stat in constructor */
class pathfilestatus
{
public:
    pathfilestatus(fs::path ipath) : path(ipath) {
        status = fs::symlink_status(path);
    };
    fs::file_status getstatus() const {
        return status;
    };
    fs::path getpath() const {
        return path;
    };

private:
    fs::path path;
    fs::file_status status;
};

// fwd
class scanner;

class directoryTree
{
public:
    /* non existing arguments are silently ignored */
    directoryTree(std::vector<fs::path> pathes, scanner *scanner, sparlib::Writer* writer);
    directoryTree(std::initializer_list<fs::path> pathes, scanner *scanner, sparlib::Writer* writer);
    void fillTree();
    std::mutex childlock;
    std::map<fs::path, directoryTree*> children;
    sparlib::Writer* writer;

private:
    // std::vector<fs::directory_entry> entries;
    std::vector<pathfilestatus> entries;
    scanner* callingscanner;
};

struct qobject {
    pathfilestatus path;
    directoryTree *dtree;
};

class scanner
{
public:
    scanner(int threads);
	void scan(pathfilestatus entry,directoryTree *dt);
    void finish();


private:
    void static scannerWorker(scanner *t);
    std::vector<std::thread*> threadlist;
    int numthreads; 
	std::deque<qobject> queue;
	std::mutex qmutex;
	std::condition_variable qcond;
    std::atomic<int> active;
};

#endif