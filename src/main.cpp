#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <Context.hpp>

namespace po = boost::program_options;
using namespace std;

po::options_description Options("Options");
po::options_description Hidden("Hidden");
po::options_description All("All");
po::positional_options_description PosDesc;

void usage() {
    cout << "Usage: Z3 [options] <input>\n";
    cout << Options << endl;
}

int main(int argc, char** argv) {
    Context ExecContent;
    int StepToReach = 4;
    int PrintFrom = 0;
    string ToErase = "0,0";
    int EraseFirst, EraseLast;
    string FileName = "";

    Options.add_options()
        ("help,h", "show this help message")
        //("verbose,v", "set verbose mode on")
        ("show-unimplemented,u", "show unimplement ZAM instructions")
        ("step,s", po::value<int>(&StepToReach)->default_value(StepToReach), "set step to reach\n    1: Reading of instructions\n    2: CFG generation\n    3: llvm code generation\n    4: llvm code execution")
        ("from,f", po::value<int>(&PrintFrom)->default_value(PrintFrom), "specify the code offset to which the generation will start")
        ("erase,e", po::value< string >(&ToErase)->default_value(ToErase), "specify a range of code offset to erase (2 values expected)\n    positive: from the begining\n    negative: from the end")
        ;

    Hidden.add_options()
        ("input-file", po::value<string>(&FileName), "input file")
        ;

    All.add(Options).add(Hidden);
    PosDesc.add("input-file", -1);

    po::variables_map VM;
    try {
        po::store(po::command_line_parser(argc, argv).
                  options(All).positional(PosDesc).run(), VM);
        po::notify(VM);
    } catch (const exception& e) {
        cout << e.what() << endl;
        usage();
        return 1;
    }

    if (VM.count("help")) { 
        usage();
        return 0;
    }

    if (VM.count("show-unimplemented")) { 
        cout << "Not yet implemented\n";
        return 0;
    }

    if (FileName == "") {
        cout << "Input file missing\n";
        usage();
        return 1;
    }

    if (sscanf(ToErase.c_str(), "%d,%d", &EraseFirst, &EraseLast) != 2 ||
        EraseFirst > EraseLast) {
        cout << "Range to erase malformed\n";
        usage();
        return 1;
    }

    ExecContent.init(FileName, PrintFrom, EraseFirst, EraseLast);
    if (StepToReach > 1) ExecContent.generateMod();
    if (StepToReach > 2) ExecContent.compile();
    if (StepToReach > 3) ExecContent.exec();
}
