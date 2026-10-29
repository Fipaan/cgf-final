#include "basic.hpp"

bool read_entire_file(std::string& file, const char *filename)
{
    std::ifstream sfile(filename);

    if (!sfile) {
        errorfn("Failed to open file: {}", filename);
        return false;
    }

    std::stringstream buffer;
    buffer << sfile.rdbuf();

    file = buffer.str();
    return true;
}
