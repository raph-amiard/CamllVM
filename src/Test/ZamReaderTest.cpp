#include <ZamReader.hpp>
#include <Marshal.hpp>

int main(int argc, char** argv) {
    if (argc <= 1) return 0;
    printType(0X10);
    ZamFile ZFile(argv[1]);
}
