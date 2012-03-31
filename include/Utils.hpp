#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <cstdio>
extern "C" {
    #include <unistd.h>
}

extern int DBG;
#define DEBUG(expr) {if (DBG) {expr}}

void setDBG(int Val);

inline std::string getExecutablePath() {
    char buffer[1024];
    size_t size = readlink("/proc/self/exe", buffer, 1024);
    buffer[size] = 0;
    std::string Ret = std::string(buffer);
    int SlashPos = Ret.find_last_of('/');
    Ret.resize(SlashPos+1);
    return Ret;
}

#endif
