#include "multistreamfile.h"
#include <thread>
#include <iostream>

long FLAGS=0;

int main() {

    std::cerr << "1. test" << std::endl;

    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile", 32, 2);

        msf.write("AAAAAAAAAAAAAAAAAAAA", 16, 0);

        msf.write("BBBBBBBBBBBBBBBBBBB", 16, 1);
   
        msf.write("CCCCCCCCCCCCCCCCCCCC", 16, 0);

        msf.write("DDDDDDDDDDDDDDDDDDD", 16, 1);
      
    }

    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile");
        char buffer[17];
        msf.read(buffer, 16, 0);
        buffer[16] = 0;
        std::cout << buffer << std::endl;
        msf.read(buffer, 16, 0);
        buffer[16] = 0;
        std::cout << buffer << std::endl;
        msf.read(buffer, 16, 1);
        buffer[16] = 0;
        std::cout << buffer << std::endl;
        msf.read(buffer, 16, 1);
        buffer[16] = 0;
        std::cout << buffer << std::endl;

    }
    //exit(0);

    std::cerr << "2. test" << std::endl;

    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile2", 32, 2);
        std::thread t1([&]() {
            msf.write("AAAAAAAAAAAAAAAAAAAA", 16 , 0);
        });
        std::thread t2([&]() {
            msf.write("BBBBBBBBBBBBBBBBBBB", 16 , 1);
        });
        t1.join();
        t2.join();
        std::thread t3([&]() {
            msf.write("CCCCCCCCCCCCCCCCCCCC", 16 , 0);
        });
        std::thread t4([&]() {
            msf.write("DDDDDDDDDDDDDDDDDDD", 16 , 1);
        });
        t3.join();
        t4.join();
    }

    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile2");
        char buffer[17];
        msf.read(buffer, 16, 0);
        buffer[16] = 0;
        std::cout << buffer << std::endl;
        msf.read(buffer, 16, 0);
        buffer[16] = 0;
        std::cout << buffer << std::endl;
        msf.read(buffer, 16, 1);
        buffer[16] = 0;
        std::cout << buffer << std::endl;
        msf.read(buffer, 16, 1);
        buffer[16] = 0;
        std::cout << buffer << std::endl;

    }
    //exit(0);

    std::cerr << "3. test" << std::endl;

    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile3", 32, 2);
        std::thread t1([&]() {
            msf.write("<AAAAAAAAAAAAAA>", 16 , 0);
        });
        std::thread t2([&]() {
            msf.write("<BBBBBBBBBBBBBB>", 16 , 1);
        });
        t1.join();
        t2.join();
        std::thread t3([&]() {
            msf.write("<CCCCCCCCCCCCCC>", 16 , 0);
        });
        std::thread t4([&]() {
            msf.write("<DDDDDDDDDDDDDD>", 16 , 1);
        });
        t3.join();
        t4.join();
    }


    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile3");
        char buffer[17];
        msf.read(buffer, 16, 0);
        buffer[16] = 0;
        std::cout << buffer << std::endl;
        msf.read(buffer, 16, 1);
        buffer[16] = 0;
        std::cout << buffer << std::endl;
        msf.read(buffer, 16, 0);
        buffer[16] = 0;
        std::cout << buffer << std::endl;
        msf.read(buffer, 16, 1);
        buffer[16] = 0;
        std::cout << buffer << std::endl;

    }



    return 0;
}
