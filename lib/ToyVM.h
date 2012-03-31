// -*- c-basic-offset: 2 default-tab-width: 2 indent-tabs-mode: t -*-
// vim: autoindent tabstop=2 noexpandtab shiftwidth=2 softtabstop=2
#ifndef _TOY_VM_H_
#define _TOY_VM_H_

#include <cstring>
#include "util.h"
#include "vmkit/MethodInfo.h"
#include "vmkit/VirtualMachine.h"
#include <Context.hpp>

namespace toy {
	class ToyThread;

	class ToyVM : public vmkit::VirtualMachine {
	public:
		int    argc;
		char** argv;

		std::string	 FileName;
		Context* ExecContent;
		int			 StepToReach;
		int			 EraseFrom;
		int			 EraseFirst;
		int			 EraseLast;

		ToyVM(vmkit::BumpPtrAllocator& Alloc, vmkit::CompiledFrames** frames);
		
		virtual ~ToyVM();
		
		virtual void runApplication(int argc, char** argv);

		void initialise();
	private:
		static void mainStart(ToyThread*);
		
		virtual void tracer(word_t closure);

		virtual void startCollection();
		virtual void endCollection();
		virtual void scanWeakReferencesQueue(word_t closure);
		virtual void scanSoftReferencesQueue(word_t closure);
		virtual void scanPhantomReferencesQueue(word_t closure);
		virtual void scanFinalizationQueue(word_t closure);
		virtual void addFinalizationCandidate(gc* obj);
		virtual size_t getObjectSize(gc* obj);
		virtual const char* getObjectTypeName(gc* obj);
		
		virtual bool enqueueReference(gc* _obj) {return false;}
		virtual void invokeFinalizer(gc* _obj) {}
		
		virtual void printMethod(vmkit::FrameInfo* FI, word_t ip, word_t addr);
		
		virtual void nullPointerException();
		virtual void stackOverflowError();
	};

}

#endif // protect from multiple inclusion 
