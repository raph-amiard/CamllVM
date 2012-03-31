#include "ToyVM.h"
#include "ToyThread.h"

using namespace toy;

void ToyVM::tracer(word_t closure) {
	//markAndTraceRoot(&ConstPool::True, closure);
	//markAndTrace(this, &attributes, closure);
	nyi();
}

void ToyThread::tracer(word_t closure) {
	nyi();
}
