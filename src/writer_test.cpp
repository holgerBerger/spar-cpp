
/* 
 * simple test program for archiver
 * for performance tests
 * 
 * (c) Holger Berger 2018
 */

 

#include "sparlib.h"
#include "multistreamfile.h"
#include <memory>
#include <iostream>
#include <cstring>

long FLAGS;

int main(int argc, char** argv) {
    
    //  1 = timing; 2 = detailed timing; 4 = debugging
    FLAGS=1+2+4;
    FLAGS=3;

    if (argc<=0) {
        std::cout << "usage: "  << argv[0] << " archive file1 file2 file3 " << std::endl;
        std::cout << "  or" << std::endl;
        std::cout << "usage: "  << argv[0] << " archive < filelist" << std::endl;
        exit(0);
    }

    if (argc>2) {
        auto msf = multistreamfile::MultistreamFile(argv[1], 1024*1024, 4);
        sparlib::Writer writer(&msf, 16);
        
        struct stat dummy;
        
        for(int i=2; i<argc; i++) {
            writer.addFile("/",argv[i], dummy);
        }
        writer.close();
    } else {
        auto msf = multistreamfile::MultistreamFile(argv[1], 1024*1024, 4);
        sparlib::Writer writer(&msf, 16);
        
        struct stat dummy;

        char name[2048];    

        while(1) {
            char* ret = fgets(name, 1023, stdin);
            if (ret==NULL) break;
            name[strlen(name)-1] = 0;
            writer.addFile("/",name, dummy);
        }
        
        writer.close();
    }

}
