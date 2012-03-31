// -*- c-basic-offset: 2 default-tab-width: 2 indent-tabs-mode: t -*-
// vim: autoindent tabstop=2 noexpandtab shiftwidth=2 softtabstop=2

#include "ToyThread.h"
#include "ToyVM.h"

using namespace toy;

ToyThread::ToyThread(ToyVM* vm) {
	MyVM = vm;
}

ToyThread::~ToyThread() {
}

ToyThread* ToyThread::get() {
	return static_cast<ToyThread*>(MutatorThread::get());
}

ToyVM* ToyThread::vm() {
	return static_cast<ToyVM*>(MyVM);
}

void ToyThread::run() {
	this->vm()->ExecContent->exec();
}
