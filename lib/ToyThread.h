// -*- c-basic-offset: 2 default-tab-width: 2 indent-tabs-mode: t -*-
// vim: autoindent tabstop=2 noexpandtab shiftwidth=2 softtabstop=2

#ifndef _TOY_THREAD_H_
#define _TOY_THREAD_H_

#include "MutatorThread.h"

#include "vmkit/System.h"
#include "vmkit/Locks.h"

namespace toy {
	class ToyVM;

	class ToyThread : public vmkit::MutatorThread {
	public:
		ToyThread(ToyVM* vm);
		virtual ~ToyThread(); /* TODO: Must verify that this destructor is called when a thread die */

		ToyVM*     vm();
		ToyThread* get();

		virtual void tracer(word_t closure);

		void run();
	};
}

#endif
