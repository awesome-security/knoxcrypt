#ifndef BFS_TEST_HELPERS_HPP__
#define BFS_TEST_HELPERS_HPP__

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <string>

int const HELLO_IT = 1000;
int const BIG_SIZE = HELLO_IT * 13;


inline boost::filesystem::path buildImage(boost::filesystem::path const &path, long const blockCount)
{
    std::string testImage(boost::filesystem::unique_path().string());
    boost::filesystem::path testPath = path / testImage;
    bfs::MakeBFS bfs(testPath.string(), blockCount);
    return testPath;
}

std::string createLargeStringToWrite()
{
    std::string theString("");
    for (int i = 0; i < HELLO_IT; ++i) {
        theString.append("Hello, World!");
    }
    return theString;
}

#define ASSERT_EQUAL(A, B, C)                   \
    if(A == B) {                                \
        std::cout<<C<<"...passed"<<std::endl;   \
    } else {                                    \
        std::cout<<C<<"...failed"<<std::endl;   \
    }

#endif
