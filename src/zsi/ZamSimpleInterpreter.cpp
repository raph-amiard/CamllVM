#include <Instructions.hpp>

namespace zsi { // aka zam_simple_interpreter

  void run(std::vector< ZInstruction* > *Instructions) {
      std::cerr << "inside zsi::run !: '" << Instructions->size()
                << "' instructions" << std::endl;

      //printInstructions(*Instructions, true, 0);

      exit(0);
  }

}
