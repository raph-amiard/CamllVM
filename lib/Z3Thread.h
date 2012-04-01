// -*- c-basic-offset: 2 default-tab-width: 2 indent-tabs-mode: t -*-
// vim: autoindent tabstop=2 noexpandtab shiftwidth=2 softtabstop=2

#ifndef _Z3_THREAD_H_
#define _Z3_THREAD_H_

#include "MutatorThread.h"

#include "vmkit/System.h"
#include "vmkit/Locks.h"

namespace z3 {
	class Z3VM;

	class Z3Thread : public vmkit::MutatorThread {
	public:
		Z3Thread(Z3VM* vm);
		virtual ~Z3Thread(); /* TODO: Must verify that this destructor is called when a thread die */

		Z3VM*     vm();
		Z3Thread* get();

		virtual void tracer(word_t closure);

		void run();
	};
}

#endif
