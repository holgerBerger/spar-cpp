/*
 * spar scanner
 * 
 * fast parallel filesystem scanner
 * avoids stats
 * depends on boost filesystem
 *
 * TODO:
 * - make std::filesystem an option for c++17
 * - make a version without boost or std::filesystem that uses posix calls or a wrapper
 *  
 * (c) Holger Berger 2018
 */


#define BOOST_FILESYSTEM_NO_DEPRECATED

#include <boost/filesystem.hpp>

#include "scanner.h"
#include <iostream>
#include <thread>

#include "sparlib.h"

extern long FLAGS;

directoryTree::directoryTree(std::initializer_list<fs::path> pathes, scanner* scanner, sparlib::Writer* writer) {
    directoryTree(std::vector<fs::path>(pathes), scanner, writer);
};

// initialize top level and fill the tree, ignore non-existing pathes given as arguments 
directoryTree::directoryTree(std::vector<fs::path> pathes, scanner *callingscanner, sparlib::Writer* writer)  {
    this->callingscanner = callingscanner;
    this->writer = writer;
    for(auto &p : pathes) {
            auto helper = pathfilestatus(p);
            if(fs::exists(helper.getstatus()))
                entries.push_back(helper);
    }  
    fillTree();
};


// go over entries and descend into directories, recursive
void directoryTree::fillTree() { 
    struct stat dummy;
    for(auto &i: entries) {
        
        // if(FLAGS&8) std::cout << i.getpath().string() << std::endl;
        writer->addFile("/",i.getpath().string(), dummy);

        if(fs::is_directory(i.getstatus())) {
            callingscanner->scan(i, this);
        }
    }
}


// create threads
scanner::scanner(int threads) : numthreads(threads)
{
    // spawn workerthreads
    active=1; // one means do not exit threads
    for(int i=0; i<numthreads; i++) {
        threadlist.push_back( new std::thread(scannerWorker,this) );
    }
}

// threaded worker in pool
void scanner::scannerWorker(scanner *t) {
   	while(true) {
		// get entry from input queue
		std::unique_lock<std::mutex> lock(t->qmutex);
		t->qcond.wait(lock, [t] {return !t->queue.empty();});
		if (t->active==0) break; // exit from thread
		auto entry = t->queue.front();
		t->active++;
		t->queue.pop_front();
		lock.unlock();

		// process it

		std::vector<fs::path> v;
		try
		{
			for (auto &&x : fs::directory_iterator(entry.path.getpath())) {
				v.push_back(x.path());
				std::this_thread::yield();
			}
		}
		catch (fs::filesystem_error)
		{
			std::cerr << "error for " << entry.path.getpath() << std::endl;
		}
		std::lock_guard<std::mutex> lock2(entry.dtree->childlock);
		entry.dtree->children[entry.path.getpath()] = new directoryTree(v, t, entry.dtree->writer);
		t->active--;
	}   
}

// push one entry down the queue
void scanner::scan(pathfilestatus entry, directoryTree *dt) {
	std::lock_guard<std::mutex> lock(qmutex);
    queue.push_back({entry,dt});
    qcond.notify_one();
}

// wait for workers to finish
void scanner::finish() {
    // we are done when there is nothing left to do and nobody is working anymore
    while(true) {
        // be nice while polling
        std::this_thread::yield();
        usleep(1000);
        if (active==1 && queue.empty()) break;        
    }
    active=0; // exit threads
    queue.push_back({pathfilestatus(""),nullptr});
    qcond.notify_all(); // wake them up to exit
}


/* simple test 
int main(int argc, char **argv) {
    std::vector<fs::path> args;


    int threads = 4;

    for(int i=1; i<argc; i++) {
        args.push_back(fs::path(argv[i]));
    }

    scanner scanner(threads);
    directoryTree dt(args, &scanner, [](std::string f) {std::cout << f << std::endl;} );
    scanner.finish();
    
}
*/
