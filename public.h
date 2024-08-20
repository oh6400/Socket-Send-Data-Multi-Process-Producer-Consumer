#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <experimental/filesystem>

#define INPUT_PORT 5005
#define FILE_PATH_LENGTH 1024

using namespace std;
namespace fs = std::experimental::filesystem;