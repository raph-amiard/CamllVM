// -*- c-basic-offset: 2 default-tab-width: 2 indent-tabs-mode: t -*-
// vim: autoindent tabstop=2 noexpandtab shiftwidth=2 softtabstop=2

#include "Z3Thread.h"
#include "Z3VM.h"

using namespace z3;

Z3Thread::Z3Thread(Z3VM* vm) {
	MyVM = vm;
}

Z3Thread::~Z3Thread() {
}

Z3Thread* Z3Thread::get() {
	return static_cast<Z3Thread*>(MutatorThread::get());
}

Z3VM* Z3Thread::vm() {
	return static_cast<Z3VM*>(MyVM);
}

void Z3Thread::run() {
	this->vm()->ExecContent->exec();
}
