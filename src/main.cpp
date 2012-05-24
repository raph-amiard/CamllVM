#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <Context.hpp>
#include <Utils.hpp>

namespace po = boost::program_options;
using namespace std;

extern "C" {
    void caml_sys_init(char * exe_name, char **argv);
}


po::options_description Options("Options");
po::options_description Hidden("Hidden");
po::options_description All("All");
po::positional_options_description PosDesc;

void usage() {
    cout << "Usage: Z3 [options] <input>\n";
    cout << Options << endl;
}

int main(int argc, char** argv) {

    caml_sys_init(argv[0], argv);

    int StepToReach = 4;
    int PrintFrom = 0;
    bool PrintTime = false;
    string ToErase = "0,0";
    int EraseFirst, EraseLast;
    string FileName = "";
    int ModeContext = 0;

    Options.add_options()
        ("help,h", "Show this help message.")
        //("verbose,v", "set verbose mode on")
        ("show-unimplemented,u", "Show unimplemented ZAM instructions.")
        ("step,s", po::value<int>(&StepToReach)->default_value(StepToReach), "Set step to reach:\n    1: Reading of instructions\n    2: CFG generation\n    3: llvm code generation\n    4: llvm code execution")
        ("from,f", po::value<int>(&PrintFrom)->default_value(PrintFrom), "Specify the code offset from which the generation will start.")
        ("erase,e", po::value< string >(&ToErase)->default_value(ToErase), "Specify a range of code offset to erase (2 values expected)\n    positive: from the begining\n    negative: from the end")
        ("verbose,v", "Show debug messages\n")
        ("time,t", "Print execution time in seconds on stderr")
        ("mode,m", po::value<int>(&ModeContext)->default_value(ModeContext), "Specify the running mode:\n    0: register based\n    1: interpreter based\n    x: same as '0'")
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

    if (VM.count("verbose")) setDBG(1);

    if (VM.count("time")) PrintTime = true;

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

    Context *ExecContent = nullptr;
    if (ModeContext != 1) {
        ExecContent = new Context();
    } else {
        ExecContent = new SimpleContext();
    }

    ExecContent->init(FileName, PrintFrom, EraseFirst, EraseLast);
    if (StepToReach > 1) ExecContent->generateMod();
    if (StepToReach > 2) ExecContent->compile();
    if (StepToReach > 3) ExecContent->exec(PrintTime);
}
