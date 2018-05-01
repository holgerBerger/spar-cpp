/*
 * spar   Simple Parallel ARchiver
 *  
 * main driver
 * 
 * TODO
 * options for compression/checksums
 * code for extraction
 * make sure existing archive does not get archived (delete it?)
 * 
 * (c) Holger Berger 2018
 */

#include <iostream>
#include <thread>
#include <vector>
#include <boost/program_options.hpp>

#include "sparlib.h"
#include "scanner.h"

namespace po = boost::program_options;

//  1 = timing; 2 = detailed timing; 4 = debugging; 8 = verbose
long FLAGS;

int main(int argc, char **argv) {
    std::string archivename;
    bool input = false;
    unsigned int streams, threads;
    po::variables_map opt;
    unsigned int hwthreads = std::thread::hardware_concurrency();
    unsigned int buffersize;

    po::options_description cmd_options( "\nOptions" );
    cmd_options.add_options()
            ("help,h", "produce help message")
            ("create,c", po::value<std::string>(&archivename), "create archive")
            ("extract,e", po::value<std::string>(&archivename), "extract archive")
            ("streams,s", po::value<unsigned int>(&streams)->default_value(2), "streams in archive file")
            ("parallel,p", po::value<unsigned int>(&threads)->default_value(hwthreads), "parallel threads")
            ("buffersize,b", po::value<unsigned int>(&buffersize)->default_value(1024), "IO buffer size in KB")
            ("input,i", "read filelist from stdin")
            ("verbose,v", "verbose information")
            ("debug,d", "debugging information")
            ("timing,t", "print timing information")
            ("Timing,T", "print detailed timing information")
            ("input-files", po::value<std::vector<std::string>>(), "Input files")
    ;

    po::positional_options_description p;
    p.add("input-files", -1);

    // parse commandline
    try{
        po::store(po::command_line_parser(argc, argv).options(cmd_options).positional(p).run(), opt);
        po::notify(opt);
    } catch (...) {
        std::cout << "Usage: " << argv[0] << ": [options]" << std::endl;
        std::cout << cmd_options << std::endl;
        exit(1);
    }

    // interpret commandline
    if (opt.count("help")) {
        std::cout << "Usage: " << argv[0] << ": [options]" << std::endl;
        std::cout << cmd_options << std::endl;
        exit(1);
    }

    if (opt.count("verbose")) FLAGS += 8;
    if (opt.count("debug")) FLAGS += 4;
    if (opt.count("Timing")) FLAGS += 2;
    if (opt.count("timing")) FLAGS += 1;

    if (opt.count("input")) input = true;

    if (opt.count("create")) {

        // no scanner
        if (input) {
            
            auto msf = multistreamfile::MultistreamFile(archivename, buffersize*1024, streams);
            sparlib::Writer writer(&msf, threads);

            struct stat dummy;
            char name[2048];    
            // read filenames from stdin
            while(1) {
                char* ret = fgets(name, 1023, stdin);
                if (ret==NULL) break;
                name[strlen(name)-1] = 0; // remove \n
                writer.addFile("/",name, dummy);
            }
            
            writer.close();

        } else { // we need a parallel scanner

            if(opt.count("input-files")){

                auto msf = multistreamfile::MultistreamFile(archivename, buffersize*1024, streams);
                sparlib::Writer writer(&msf, threads);

                std::vector<std::string> files = opt["input-files"].as<std::vector<std::string>>();
                std::vector<fs::path> args;           
                for(auto f: files) {
                    args.push_back(fs::path(f));
                }
                scanner scanner(threads);
                directoryTree dt(args, &scanner, &writer );
                // wait for scanning to finish
                scanner.finish();
                // close output file (this blocks)
                writer.close();

            } else {
                std::cerr << "nothing to do!" << std::endl;
            }

        }
    }
    
}
