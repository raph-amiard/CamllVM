#include <Context.hpp>

int main(int argc, char** argv) {
    if (argc <= 1) return 0;
    Context ExecContext;
    ExecContext.init(argv[1]);
}
