PROJECT(spar++)

cmake_minimum_required(VERSION 2.8)

SET(CMAKE_CXX_COMPILER "/opt/latest_llvm/bin/clang++")
SET(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")

SET(CMAKE_CXX_FLAGS "-O0 -Wall --std=c++11 -g")
#SET(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=gold")

find_package(Boost COMPONENTS system filesystem program_options REQUIRED)


SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${spar++_BINARY_DIR}/bin)


ADD_EXECUTABLE(multistreamfile_test ${spar++_SOURCE_DIR}/src/multistreamfile_test.cpp
                                    ${spar++_SOURCE_DIR}/src/multistreamfile.cpp
)

ADD_EXECUTABLE(writer_test 	${spar++_SOURCE_DIR}/src/writer_test.cpp
				${spar++_SOURCE_DIR}/src/multistreamfile.cpp
				${spar++_SOURCE_DIR}/src/sparlib.cpp
)

ADD_EXECUTABLE(spar_analyzer 	${spar++_SOURCE_DIR}/src/spar_analyzer.cpp
				${spar++_SOURCE_DIR}/src/multistreamfile.cpp
				${spar++_SOURCE_DIR}/src/sparlib.cpp
)

#ADD_EXECUTABLE(scanner ${spar++_SOURCE_DIR}/src/scanner.cpp)

ADD_EXECUTABLE(spar    ${spar++_SOURCE_DIR}/src/spar.cpp
                ${spar++_SOURCE_DIR}/src/multistreamfile.cpp
                ${spar++_SOURCE_DIR}/src/sparlib.cpp
                ${spar++_SOURCE_DIR}/src/scanner.cpp
)


TARGET_LINK_LIBRARIES(multistreamfile_test pthread)
TARGET_LINK_LIBRARIES(writer_test pthread)
TARGET_LINK_LIBRARIES(spar_analyzer pthread)
#TARGET_LINK_LIBRARIES(scanner   ${Boost_FILESYSTEM_LIBRARY}
				#${Boost_SYSTEM_LIBRARY}
				#pthread)
TARGET_LINK_LIBRARIES(spar ${Boost_LIBRARIES} pthread)


enable_testing()

# indicates the location of the boost installation tree.
# hard-coded for our simple example.
#set(BOOST_INCLUDE_DIRS $boost_installation_prefix/include)
FIND_PACKAGE(Boost COMPONENTS unit_test_framework REQUIRED)


# creates the executable
add_executable(test_multistream ${spar++_SOURCE_DIR}/src/test_multistream.cpp
				${spar++_SOURCE_DIR}/src/multistreamfile.cpp
)

# indicates the include paths
target_include_directories(test_multistream PRIVATE ${BOOST_INCLUDE_DIRS})
target_compile_definitions(test_multistream PRIVATE "BOOST_TEST_DYN_LINK=1")
target_link_libraries(test_multistream ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} pthread)

# declares a test with our executable
add_test(NAME test_multistream COMMAND test_multistream)
