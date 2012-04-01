// -*- c-basic-offset: 2 default-tab-width: 2 indent-tabs-mode: t -*-
// vim: autoindent tabstop=2 noexpandtab shiftwidth=2 softtabstop=2

#include "Z3VM.h"
#include "Z3Thread.h"

using namespace z3;

void Z3VM::initialise() {
	this->ExecContent->init(this->FileName, this->EraseFrom, this->EraseFirst, this->EraseLast);
	if (this->StepToReach > 1) this->ExecContent->generateMod();
	if (this->StepToReach > 2) this->ExecContent->compile();
	if (this->StepToReach == 3)
		exit();
}

Z3VM::Z3VM(vmkit::BumpPtrAllocator& Alloc, vmkit::CompiledFrames** frames) :
	VirtualMachine(Alloc, frames) {
	}

Z3VM::~Z3VM() {
}

void Z3VM::runApplication(int argc, char** argv) {
	this->argc = argc;
	this->argv = argv;
	mainThread = new Z3Thread(this);
	mainThread->start((void (*)(vmkit::Thread*))mainStart);
}

void Z3VM::mainStart(Z3Thread* self) {
	std::cout << "vm init\n";
	self->vm()->initialise();
	std::cout << "thread run\n";
	self->run();
	std::cout << "vm exit\n";
	self->vm()->exit();
}

void Z3VM::startCollection() {
	debug_printf("Starting a collection\n");
}

void Z3VM::endCollection() {
	debug_printf("Collection finished\n");
}

void Z3VM::scanWeakReferencesQueue(word_t closure) {
	nyi();
}

void Z3VM::scanSoftReferencesQueue(word_t closure) {
	nyi();
}

void Z3VM::scanPhantomReferencesQueue(word_t closure) {
	nyi();
}

void Z3VM::scanFinalizationQueue(word_t closure) {
	nyi();
}

void Z3VM::addFinalizationCandidate(gc* obj) {
	nyi();
}

size_t Z3VM::getObjectSize(gc* obj) {
	nyi();
	return 0;
}

const char* Z3VM::getObjectTypeName(gc* obj) {
	nyi();
	return (char*)NULL;
}

void Z3VM::printMethod(vmkit::FrameInfo* FI, word_t ip, word_t addr) {
	nyi();
}

void Z3VM::nullPointerException() {
	nyi();
}

void Z3VM::stackOverflowError() {
	nyi();
}
