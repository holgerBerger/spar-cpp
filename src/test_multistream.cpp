#define BOOST_TEST_MODULE multistream
#include <boost/test/unit_test.hpp>

#include "multistreamfile.h"
#include <string>
#include <iostream>
#include <thread>

long FLAGS;


BOOST_AUTO_TEST_SUITE( serial )
BOOST_AUTO_TEST_CASE( multistream_serial ) {
    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile", 32, 1);
	msf.lock(0);
        msf.write("AAAAAAAAAAAAAAAAAAAA", 16, 0);
	msf.unlock(0);
	msf.lock(0);
        msf.write("BBBBBBBBBBBBBBBBBBB", 16, 0);
	msf.unlock(0);
	msf.lock(0);
        msf.write("CCCCCCCCCCCCCCCCCCCC", 16, 0);
	msf.unlock(0);
	msf.lock(0);
        msf.write("DDDDDDDDDDDDDDDDDDD", 16, 0);
	msf.unlock(0);

    }

    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile");
        char buffer[17];
        buffer[16]=0; 
	msf.lock(0);
        msf.read(buffer, 16, 0);
	msf.unlock(0);
        BOOST_CHECK(std::string(buffer) == std::string("AAAAAAAAAAAAAAAA"));
        buffer[16]=0; 
	msf.lock(0);
        msf.read(buffer, 16, 0);
	msf.unlock(0);
        BOOST_CHECK(std::string(buffer) == std::string("BBBBBBBBBBBBBBBB"));
        buffer[16]=0; 
	msf.lock(0);
        msf.read(buffer, 16, 0);
	msf.unlock(0);
        BOOST_CHECK(std::string(buffer) == std::string("CCCCCCCCCCCCCCCC"));
        buffer[16]=0; 
	msf.lock(0);
        msf.read(buffer, 16, 0);
	msf.unlock(0);
        BOOST_CHECK(std::string(buffer) == std::string("DDDDDDDDDDDDDDDD"));
    }
}

BOOST_AUTO_TEST_CASE( multistream_2stream ) {
    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile", 32, 2);
	msf.lock(0);
        msf.write("AAAAAAAAAAAAAAAAAAAA", 16, 0);
	msf.unlock(0);
	msf.lock(1);
        msf.write("BBBBBBBBBBBBBBBBBBB", 16, 1);
	msf.unlock(1);
	msf.lock(0);
        msf.write("CCCCCCCCCCCCCCCCCCCC", 16, 0);
	msf.unlock(0);
	msf.lock(1);
        msf.write("DDDDDDDDDDDDDDDDDDD", 16, 1);
	msf.unlock(1);

    }

    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile");
        char buffer[17];
        buffer[16]=0; 
	msf.lock(0);
        msf.read(buffer, 16, 0);
	msf.unlock(0);
        BOOST_CHECK(std::string(buffer) == std::string("AAAAAAAAAAAAAAAA"));
        buffer[16]=0; 
	msf.lock(1);
        msf.read(buffer, 16, 1);
	msf.unlock(1);
        BOOST_CHECK(std::string(buffer) == std::string("BBBBBBBBBBBBBBBB"));
        buffer[16]=0; 
	msf.lock(0);
        msf.read(buffer, 16, 0);
	msf.unlock(0);
        BOOST_CHECK(std::string(buffer) == std::string("CCCCCCCCCCCCCCCC"));
        buffer[16]=0; 
	msf.lock(1);
        msf.read(buffer, 16, 1);
	msf.unlock(1);
        BOOST_CHECK(std::string(buffer) == std::string("DDDDDDDDDDDDDDDD"));
    }
}
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( parallel )
BOOST_AUTO_TEST_CASE( multistream_parallel ) {

    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile", 32, 2);

        std::thread t1([&]() {
	    msf.lock(0);
            msf.write("<AAAAAAAAAAAAAA>", 16 , 0);
	    msf.unlock(0);
        });
        std::thread t2([&]() {
	    msf.lock(1);
            msf.write("<BBBBBBBBBBBBBB>", 16 , 1);
	    msf.unlock(1);
        });
        t1.join();
        t2.join();

        std::thread t3([&]() {
	    msf.lock(0);
            msf.write("<CCCCCCCCCCCCCC>", 16 , 0);
	    msf.unlock(0);
        });
        std::thread t4([&]() {
	    msf.lock(1);
            msf.write("<DDDDDDDDDDDDDD>", 16 , 1);
	    msf.unlock(1);
        });
        t3.join();
        t4.join();

    }

    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile");
        char buffer[17];
        buffer[16]=0; 
	msf.lock(0);
        msf.read(buffer, 16, 0);
	msf.unlock(0);
        BOOST_CHECK(std::string(buffer) == std::string("<AAAAAAAAAAAAAA>"));
        buffer[16]=0; 
	msf.lock(1);
        msf.read(buffer, 16, 1);
	msf.unlock(1);
        BOOST_CHECK(std::string(buffer) == std::string("<BBBBBBBBBBBBBB>"));
        buffer[16]=0; 
	msf.lock(0);
        msf.read(buffer, 16, 0);
	msf.unlock(0);
        BOOST_CHECK(std::string(buffer) == std::string("<CCCCCCCCCCCCCC>"));
        buffer[16]=0; 
	msf.lock(1);
        msf.read(buffer, 16, 1);
	msf.unlock(1);
        BOOST_CHECK(std::string(buffer) == std::string("<DDDDDDDDDDDDDD>"));
    }
}


BOOST_AUTO_TEST_CASE( multistream_parallel_nosynch ) {

    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile", 32, 2);

        std::thread t1([&]() {
	    msf.lock(0);
            msf.write("<AAAAAAAAAAAAAA>", 16 , 0);
	    msf.unlock(0);
        });
        std::thread t2([&]() {
	    msf.lock(0);
            msf.write("<BBBBBBBBBBBBBB>", 16 , 0);
	    msf.unlock(0);
        });


        std::thread t3([&]() {
	    msf.lock(1);
            msf.write("<CCCCCCCCCCCCCC>", 16 , 1);
	    msf.unlock(1);
        });
        std::thread t4([&]() {
	    msf.lock(1);
            msf.write("<DDDDDDDDDDDDDD>", 16 , 1);
	    msf.unlock(1);
        });
        t1.join();
        t2.join();
        t3.join();
        t4.join();

    }

    {
        auto msf = multistreamfile::MultistreamFile("/tmp/testfile");
        char buffer[17];
        buffer[16]=0; 
	msf.lock(0);
        msf.read(buffer, 16, 0);
	msf.unlock(0);
        BOOST_CHECK(std::string(buffer) == std::string("<AAAAAAAAAAAAAA>") || std::string(buffer) == std::string("<BBBBBBBBBBBBBB>"));
        buffer[16]=0; 
	msf.lock(0);
        msf.read(buffer, 16, 0);
	msf.unlock(0);
        BOOST_CHECK(std::string(buffer) == std::string("<BBBBBBBBBBBBBB>") || std::string(buffer) == std::string("<AAAAAAAAAAAAAA>"));
        buffer[16]=0; 
	msf.lock(1);
        msf.read(buffer, 16, 1);
	msf.unlock(1);
        BOOST_CHECK(std::string(buffer) == std::string("<CCCCCCCCCCCCCC>") || std::string(buffer) == std::string("<DDDDDDDDDDDDDD>"));
        buffer[16]=0; 
	msf.lock(1);
        msf.read(buffer, 16, 1);
	msf.unlock(1);
        BOOST_CHECK(std::string(buffer) == std::string("<DDDDDDDDDDDDDD>") || std::string(buffer) == std::string("<CCCCCCCCCCCCCC>"));
    }
}

BOOST_AUTO_TEST_SUITE_END()
