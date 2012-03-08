//#include "mvm/VMKit.h"
#include "mvm/VirtualMachine.h"
#include "mvm/MethodInfo.h"
#include "mvm/Threads/Locks.h"
//#include "mvm/Threads/Thread.h"

#define NI() fprintf(stderr, "Not implemented\n"); abort()

mvm::CompiledFrames* frametables[] = {
  NULL
};


namespace jouetvm {

    class JouetVM : public mvm::VirtualMachine {
    public:
	JouetVM(mvm::BumpPtrAllocator &alloc, mvm::CompiledFrames** frames) : mvm::VirtualMachine(alloc, frames) {}
	~JouetVM(){ NI(); }
	/*virtual mvm::gc** getReferent(mvm::gc*) { NI(); }
	virtual void setReferent(mvm::gc*, mvm::gc*) { NI(); }
	virtual bool enqueueReference(mvm::gc*) { NI(); }*/
	virtual size_t getObjectSize(gc* object) { NI(); }
	virtual void printMethod(mvm::FrameInfo* FI, word_t ip, word_t addr) { NI(); }
	virtual void runApplication(int, char**) { NI(); }
    };

}

using namespace jouetvm;

int main(int argc, char **argv) {
    /*
    mvm::BumpPtrAllocator *Allocator = new mvm::BumpPtrAllocator();
    mvm::CompiledFrames *Frames = new mvm::CompiledFrames();
    //mvm::VMKit* vmkit = new(Allocator, "VMKit") mvm::VMKit(Allocator);

    JouetVM(Allocator, &Frames);
    */

    mvm::BumpPtrAllocator Allocator;
    JouetVM *vm = new(Allocator, "VM") JouetVM(Allocator, frametables);

    return 0;
}
