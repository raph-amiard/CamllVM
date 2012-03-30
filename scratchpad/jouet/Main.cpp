//===------------- Main.cpp - Simple execution of J3 ----------------------===//
//
//                          The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "MvmGC.h"
#include "mvm/JIT.h"
//#include "mvm/MethodInfo.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"
#include "mvm/Threads/Locks.h"
#include "MutatorThread.h"

//#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"

using namespace mvm;

#include "FrametablesExterns.inc"

mvm::CompiledFrames* frametables[] = {
    #include "FrametablesSymbols.inc"
  NULL
};


#include <iomanip>
#include <ostream>
#include <string>

struct hexdump {
    void const* data;
    int len;
    hexdump(void const* data, int len) : data(data), len(len) {}

    template<class T>
    hexdump(T const& v) : data(&v), len(sizeof v) {}

    friend
    std::ostream& operator<<(std::ostream& s, hexdump const& v) {
	// don't change formatting for s
	std::ostream out (s.rdbuf());
	out << std::hex << std::setfill('0');
	unsigned char const* pc = reinterpret_cast<unsigned char const*>(v.data);
	std::string buf;
	buf.reserve(17); // premature optimization
	int i;
	for (i = 0; i < v.len; ++i, ++pc) {
	    if ((i % 16) == 0) {
		if (i) {
		    out << "  " << buf << '\n';
		    buf.clear();
		}
		out << "  " << std::setw(4) << i << ' ';
	    }
	    out << ' ' << std::setw(2) << unsigned(*pc);
	    buf += (0x20 <= *pc && *pc <= 0x7e) ? *pc : '.';
	}
	if (i % 16) {
	    char const* spaces16x3 = "                                                ";
	    out << &spaces16x3[3 * (i % 16)];
	}
	out << "  " << buf << '\n';
	return s;
    }
};

//#include "../../lib/Jouet/MyCore/Myjvm.h"
//using namespace jouet;


class OcamlHeader : public gcRoot {
public:
    int u;
    int v;
    void tracer(word_t closure) {
	std::cout << "OcamlHeader::tracer" << std::endl;
	return;
    }

    OcamlHeader *omalloc(uint32_t sz) {
	OcamlHeader *res = NULL;
	res = (OcamlHeader *) gcmalloc(sz, this->getVirtualTable());
	return res;
    }
};

class OcamlThread : public mvm::MutatorThread {
public:
    OcamlThread(mvm::VirtualMachine *isolate) : MutatorThread() {
	MyVM = isolate;
	
    };
};


class Ovm : public mvm::VirtualMachine {
public:
    Ovm(mvm::BumpPtrAllocator &Alloc, mvm::CompiledFrames** frames) :
	mvm::VirtualMachine(Alloc, frames) { std::cout << "Ovm created" << std::endl; };

    ~Ovm() {};

    static void my_main_function(OcamlThread *torg) { 
	std::cout << ">>> my_main_function; TotalMemory=" << Collector::getTotalMemory() << std::endl;


	while (1) {
	    std::cout << "??? my_main_function; TotalMemory=" << Collector::getTotalMemory()
		      << std::endl;
	    Collector::collect();

	    OcamlHeader patron = OcamlHeader();
	    OcamlHeader *x = NULL;
	    //truc((void**) &x); //llvm_gcroot(x, 0);	
	    x = patron.omalloc(32);
	    std::cerr << "x~32 @" << x << ":\n" << hexdump(x, 32);
	    
	    OcamlHeader *y = NULL;
	    //truc((void**) &y); //llvm_gcroot(y, 0);
	    y = patron.omalloc(32);
	    //y->header = 7;
	    y->u = 3;
	    y->v = std::numeric_limits<int>::max();
	    std::cerr << "y~32 @"<< y << ":\n" << hexdump(y, 32);
	    //break;
	    sleep(2);
	};

	std::cout << "<<< my_main_function; TotalMemory=" << Collector::getTotalMemory() << std::endl;
	return;
    };

    void startCollection() {
	std::cout << "startCollection" << std::endl;
    }

    void runApplication(int argc, char** argv) {
	std::cout << "runApplication" << std::endl;
	mainThread = new OcamlThread(this);
	std::cout << "starting mainThread" << std::endl;
	mainThread->start((void (*)(mvm::Thread*)) my_main_function);
    };

    size_t getObjectSize(gc* object) { return 0; };
    void printMethod(mvm::FrameInfo* FI, word_t ip, word_t addr) { return; };

};

int main(int argc, char **argv, char **envp) {
    
  llvm::llvm_shutdown_obj X;
  
  // Initialize base components.  
  //MvmModule::initialise(argc, argv);
  
  Collector::initialise(argc, argv);
  Collector::verbose = 1;

  mvm::BumpPtrAllocator Allocator;
  Ovm *i = new(Allocator, "i") Ovm(Allocator, frametables);
  i->runApplication(argc, argv);
  i->waitForExit();
  i->~Ovm();
  System::Exit(0);

  return 0;
}
