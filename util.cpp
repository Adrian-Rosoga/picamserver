#include "util.h"

#include <memory>

int exec(const char* cmd, char* buffer, unsigned int bufferSize)
{
    std::shared_ptr<FILE> pipePtr(popen(cmd, "r"), pclose);
    
    if (!pipePtr) { return -1; }
    
    char* pos = buffer;
    while (!feof(pipePtr.get()) && bufferSize - (pos - buffer) > 0)
    {
        size_t readCount;
        readCount = fread(pos, 1, bufferSize - (pos - buffer), pipePtr.get());
        pos += readCount;
    }
    
    return pos - buffer;
}