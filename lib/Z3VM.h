// -*- c-basic-offset: 2 default-tab-width: 2 indent-tabs-mode: t -*-
// vim: autoindent tabstop=2 noexpandtab shiftwidth=2 softtabstop=2
#ifndef _Z3_VM_H_
#define _Z3_VM_H_

#include <cstring>
#include "util.h"
#include "vmkit/MethodInfo.h"
#include "vmkit/VirtualMachine.h"
#include <Context.hpp>

namespace z3 {
	class Z3Thread;

	class Z3VM : public vmkit::VirtualMachine {
	public:
		int    argc;
		char** argv;

		std::string	 FileName;
		Context* ExecContent;
		int			 StepToReach;
		int			 EraseFrom;
		int			 EraseFirst;
		int			 EraseLast;

		Z3VM(vmkit::BumpPtrAllocator& Alloc, vmkit::CompiledFrames** frames);
		
		virtual ~Z3VM();
		
		virtual void runApplication(int argc, char** argv);

		void initialise();
	private:
		static void mainStart(Z3Thread*);
		
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
