// -*- c-basic-offset: 2 default-tab-width: 2 indent-tabs-mode: t -*-
// vim: autoindent tabstop=2 noexpandtab shiftwidth=2 softtabstop=2

#include "ToyVM.h"
#include "ToyThread.h"

using namespace toy;

void ToyVM::initialise() {
	this->ExecContent->init(this->FileName, this->EraseFrom, this->EraseFirst, this->EraseLast);
	if (this->StepToReach > 1) this->ExecContent->generateMod();
	if (this->StepToReach > 2) this->ExecContent->compile();
	if (this->StepToReach == 3)
		exit();
}

ToyVM::ToyVM(vmkit::BumpPtrAllocator& Alloc, vmkit::CompiledFrames** frames) :
	VirtualMachine(Alloc, frames) {
	}

ToyVM::~ToyVM() {
}

void ToyVM::runApplication(int argc, char** argv) {
	this->argc = argc;
	this->argv = argv;
	mainThread = new ToyThread(this);
	mainThread->start((void (*)(vmkit::Thread*))mainStart);
}

void ToyVM::mainStart(ToyThread* self) {
	std::cout << "vm init\n";
	self->vm()->initialise();
	std::cout << "thread run\n";
	self->run();
	std::cout << "vm exit\n";
	self->vm()->exit();
}

void ToyVM::startCollection() {
	debug_printf("Starting a collection\n");
}

void ToyVM::endCollection() {
	debug_printf("Collection finished\n");
}

void ToyVM::scanWeakReferencesQueue(word_t closure) {
	nyi();
}

void ToyVM::scanSoftReferencesQueue(word_t closure) {
	nyi();
}

void ToyVM::scanPhantomReferencesQueue(word_t closure) {
	nyi();
}

void ToyVM::scanFinalizationQueue(word_t closure) {
	nyi();
}

void ToyVM::addFinalizationCandidate(gc* obj) {
	nyi();
}

size_t ToyVM::getObjectSize(gc* obj) {
	nyi();
	return 0;
}

const char* ToyVM::getObjectTypeName(gc* obj) {
	nyi();
	return (char*)NULL;
}

void ToyVM::printMethod(vmkit::FrameInfo* FI, word_t ip, word_t addr) {
	nyi();
}

void ToyVM::nullPointerException() {
	nyi();
}

void ToyVM::stackOverflowError() {
	nyi();
}
