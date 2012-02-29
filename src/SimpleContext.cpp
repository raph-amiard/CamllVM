#include <Context.hpp>


void SimpleContext::generateMod() {
  std::cout << "SimpleContext::generateMod" << std::endl;
}

void SimpleContext::compile() {
    std::cout << "SimpleContext::compile: '" << Instructions.size()
              << "' instructions" << std::endl;
}

void SimpleContext::exec() {
    std::cout << "SimpleContext::exec" << std::endl;
}

