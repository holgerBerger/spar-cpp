/*
 * test program dumping file structure of archive
 * for debugging
 */

#include "multistreamfile.h"
#include "sparlib.h"
#include <map>
#include <iostream>

#include <memory.h>

long FLAGS = 0;

std::map<int, std::string> sectionmap = {{0, "FileHeader"}, {1, "DirHeader"}, {2, "Softlink"}, {3, "FileBody"}, {4, "FileFooter"}, {5, "StreamEnd"}};

int main(int argc, char **argv)
{
    multistreamfile::MultistreamFile msf(argv[1]);

    char *buffer = new char[msf.getBuffersize()];
    bool eof = false;

    for (int stream = 0; stream < msf.getStreams(); stream++)
    {
        while (!eof)
        {
            sparlib::SectionHeader sh;
            ssize_t ret = msf.read((char *)&sh, sizeof(sh), stream);
            if (ret <= 0)
                eof = true;
            if (sh.magic != sparlib::sectionmagic) {
                std::cerr << "wrong section magic! " << sh.magic <<std::endl;
                exit(-1);
            }
            std::cout << "Section " << sectionmap[sh.type] << "(" << sh.type << ")"
                      << " in stream " << stream << " size=" << sh.size << std::endl;
            if (sh.type == sparlib::FileHeader)
            {
                struct sparlib::FileHeader fh;
                ssize_t ret = msf.read((char *)&fh, sizeof(fh), stream);
                if (ret <= 0)
                    eof = true;
                char *cfullname = new char[sh.size - sizeof(fh) + 1];
                memset(cfullname, 0, sh.size - sizeof(fh) + 1);
                ret = msf.read(cfullname, sh.size - sizeof(fh), stream);
                if (ret <= 0)
                    eof = true;
                // std::string fullname(cfullname);
                std::cout << " file:" << cfullname << " = " << fh.fileid << std::endl;
                delete[] cfullname;
            }
            if (sh.type == sparlib::FileBody)
            {
                struct sparlib::FileBody fh;
                ssize_t ret = msf.read((char *)&fh, sizeof(fh), stream);
                if (ret <= 0)
                    eof = true;
                ;
                std::cout << " " << fh.size << " bytes of file " << fh.fileid << std::endl;
                char *data = new char[fh.size];
                ret = msf.read(data, fh.size, stream);
                if (ret < 0)
                    eof = true;
                delete[] data;
            }
            if (sh.type == sparlib::FileFooter)
            {
                struct sparlib::FileFooter fh;
                ssize_t ret = msf.read((char *)&fh, sizeof(fh), stream);
                if (ret <= 0)
                    eof = true;
                std::cout << "  end of file " << fh.fileid << std::endl;
            }
            if (sh.type == sparlib::StreamEnd)
            {
                std::cout << "  end of stream" << std::endl;
                break;
            }
        }
    }

    delete[] buffer;
}
