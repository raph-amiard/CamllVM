// -*- c-basic-offset: 2 default-tab-width: 2 indent-tabs-mode: t -*-
// vim: autoindent tabstop=2 noexpandtab shiftwidth=2 softtabstop=2
//===------------ main.cpp - Simple execution of Reactor ----------------===//
//
//                          The VMKit project
//
// This file is distributed under the Purdue University Open Source
// License. See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------===//

#include "Z3VM.h"
#include "util.h"
#include "vmkit/JIT.h"
#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <Context.hpp>
#include <Utils.hpp>

namespace po = boost::program_options;
using namespace std;

using namespace z3;
using namespace vmkit;

extern "C" {
    void caml_sys_init(char * exe_name, char **argv);
}

static void usage(po::options_description Options) {
    cout << "Usage: Z3 [options] <input>\n";
    cout << Options << endl;
}

extern CompiledFrames* frametables[];

void checkOpts(Z3VM* vm, int argc, char **argv) {
	po::options_description Options("Options");
	po::options_description Hidden("Hidden");
	po::options_description All("All");
	po::positional_options_description PosDesc;

	caml_sys_init(argv[0], argv);

	int StepToReach = 4;
	int EraseFrom = 0;
	string ToErase = "0,0";
	int EraseFirst, EraseLast;
	string FileName = "";
	int ModeContext = 0;

	Options.add_options()
		("help,h", "Show this help message.")
		//("verbose,v", "set verbose mode on")
		("show-unimplemented,u", "Show unimplemented ZAM instructions.")
		("step,s", po::value<int>(&StepToReach)->default_value(StepToReach), "Set step to reach:\n    1: Reading of instructions\n    2: CFG generation\n    3: llvm code generation\n    4: llvm code execution")
		("from,f", po::value<int>(&EraseFrom)->default_value(EraseFrom), "Specify the code offset from which the generation will start.")
		("erase,e", po::value< string >(&ToErase)->default_value(ToErase), "Specify a range of code offset to erase (2 values expected)\n    positive: from the begining\n    negative: from the end")
		("verbose,v", "Show debug messages\n")
		("mode,m", po::value<int>(&ModeContext)->default_value(ModeContext), "Specify the running mode:\n    0: register based\n    1: interpreter based\n    x: same as '0'")
		;

	Hidden.add_options()
		("input-file", po::value<string>(&FileName), "input file")
		;

	All.add(Options).add(Hidden);
	PosDesc.add("input-file", -1);

	/*
	po::variables_map vmap;
	try {
		po::store(po::command_line_parser(argc, argv).
				options(All).positional(PosDesc).run(), vmap);
		po::notify(vmap);
	} catch (const exception& e) {
		cout << e.what() << endl;
		usage(Options);
		exit(1);
	}

	if (vmap.count("help")) { 
		usage(Options);
		exit(1);
	}

	if (vmap.count("show-unimplemented")) { 
		cout << "Not yet implemented\n";
		exit(1);
	}

	if (vmap.count("verbose")) setDBG(1);

	if (FileName == "") {
		cout << "Input file missing\n";
		usage(Options);
		exit(1);
	}

	*/
	setDBG(1);
	FileName = argv[1];
	if (sscanf(ToErase.c_str(), "%d,%d", &EraseFirst, &EraseLast) != 2 ||
			EraseFirst > EraseLast) {
		usage(Options);
		exit(1);
	}

	if (ModeContext != 1) {
		vm->ExecContent = new Context();
	} else {
		vm->ExecContent = new SimpleContext();
	}

	vm->StepToReach = StepToReach;
	vm->FileName = FileName;
	vm->EraseFrom = EraseFrom;
	vm->EraseFirst = EraseFirst;
	vm->EraseLast = EraseLast;
}

int main(int argc, char **argv){
	// Initialize base components.  
	VmkitModule::initialise(argc, argv);
	Collector::initialise(argc, argv);

	// Create the allocator that will allocate the bootstrap loader and the JVM.
	vmkit::BumpPtrAllocator Allocator;
	Z3VM* vm = new(Allocator, "VM") Z3VM(Allocator, frametables);

	checkOpts(vm, argc, argv);

	// Run the application. 
	vm->runApplication(argc, argv);
	vm->waitForExit();
	System::Exit(0);

	return 0;
}
