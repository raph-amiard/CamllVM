#include <ZamReader.hpp>

int main(int argc, char** argv) {
    if (argc <= 1) return 0;
    ZamFile ZFile(argv[1]);
}
