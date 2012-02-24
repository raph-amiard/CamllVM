#ifndef INSTRUCTIONS_HPP
#define INSTRUCTIONS_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <iostream>
#include <map>

#include <EndianUtils.hpp>


/**
 * Using higher order Macros to simplify declarations of structures
 * and redundancy. Technique borrowed from V8 javascript compiler
 * See http://journal.stuffwithstuff.com/2012/01/24/higher-order-macros-in-c/ for more information
 * The basic idea is that the OPCODE_NODE_LIST macro takes another macro as argument
 * and then calls it on every opcode, with it's name and arity
 */

#define OPCODE_NODE_LIST(code) \
    code(ACC0, 0) \
    code(ACC1, 0) \
    code(ACC2, 0) \
    code(ACC3, 0) \
    code(ACC4, 0) \
    code(ACC5, 0) \
    code(ACC6, 0) \
    code(ACC7, 0) \
    code(ACC, 1) \
    code(PUSH, 0) \
    code(PUSHACC0, 0) \
    code(PUSHACC1, 0) \
    code(PUSHACC2, 0) \
    code(PUSHACC3, 0) \
    code(PUSHACC4, 0) \
    code(PUSHACC5, 0) \
    code(PUSHACC6, 0) \
    code(PUSHACC7, 0) \
    code(PUSHACC, 1) \
    code(POP, 1) \
    code(ASSIGN, 1) \
    code(ENVACC1, 0) \
    code(ENVACC2, 0) \
    code(ENVACC3, 0) \
    code(ENVACC4, 0) \
    code(ENVACC, 1) \
    code(PUSHENVACC1, 0) \
    code(PUSHENVACC2, 0) \
    code(PUSHENVACC3, 0) \
    code(PUSHENVACC4, 0) \
    code(PUSHENVACC, 1) \
    code(PUSH_RETADDR, 1) \
    code(APPLY, 1) \
    code(APPLY1, 0) \
    code(APPLY2, 0) \
    code(APPLY3, 0) \
    code(APPTERM, 2) \
    code(APPTERM1, 1) \
    code(APPTERM2, 1) \
    code(APPTERM3, 1) \
    code(RETURN, 1) \
    code(RESTART, 0) \
    code(GRAB, 1) \
    code(CLOSURE, 2) \
    code(CLOSUREREC, 2) \
    code(OFFSETCLOSUREM2, 0) \
    code(OFFSETCLOSURE0, 0) \
    code(OFFSETCLOSURE2, 0) \
    code(OFFSETCLOSURE, 1) \
    code(PUSHOFFSETCLOSUREM2, 0) \
    code(PUSHOFFSETCLOSURE0, 0) \
    code(PUSHOFFSETCLOSURE2, 0) \
    code(PUSHOFFSETCLOSURE, 1) \
    code(GETGLOBAL, 1) \
    code(PUSHGETGLOBAL, 1) \
    code(GETGLOBALFIELD, 2) \
    code(PUSHGETGLOBALFIELD, 2) \
    code(SETGLOBAL, 1) \
    code(ATOM0, 0) \
    code(ATOM, 1) \
    code(PUSHATOM0, 0) \
    code(PUSHATOM, 1) \
    code(MAKEBLOCK, 2) \
    code(MAKEBLOCK1, 1) \
    code(MAKEBLOCK2, 1) \
    code(MAKEBLOCK3, 1) \
    code(MAKEFLOATBLOCK, 1) \
    code(GETFIELD0, 0) \
    code(GETFIELD1, 0) \
    code(GETFIELD2, 0) \
    code(GETFIELD3, 0) \
    code(GETFIELD, 1) \
    code(GETFLOATFIELD, 1) \
    code(SETFIELD0, 0) \
    code(SETFIELD1, 0) \
    code(SETFIELD2, 0) \
    code(SETFIELD3, 0) \
    code(SETFIELD, 1) \
    code(SETFLOATFIELD, 1) \
    code(VECTLENGTH, 0) \
    code(GETVECTITEM, 0) \
    code(SETVECTITEM, 0) \
    code(GETSTRINGCHAR, 0) \
    code(SETSTRINGCHAR, 0) \
    code(BRANCH, 1) \
    code(BRANCHIF, 1) \
    code(BRANCHIFNOT, 1) \
    code(SWITCH, 2) \
    code(BOOLNOT, 0) \
    code(PUSHTRAP, 1) \
    code(POPTRAP, 0) \
    code(RAISE, 0) \
    code(CHECK_SIGNALS, 0) \
    code(C_CALL1, 1) \
    code(C_CALL2, 1) \
    code(C_CALL3, 1) \
    code(C_CALL4, 1) \
    code(C_CALL5, 1) \
    code(C_CALLN, 2) \
    code(CONST0, 0) \
    code(CONST1, 0) \
    code(CONST2, 0) \
    code(CONST3, 0) \
    code(CONSTINT, 1) \
    code(PUSHCONST0, 0) \
    code(PUSHCONST1, 0) \
    code(PUSHCONST2, 0) \
    code(PUSHCONST3, 0) \
    code(PUSHCONSTINT, 1) \
    code(NEGINT, 0) \
    code(ADDINT, 0) \
    code(SUBINT, 0) \
    code(MULINT, 0) \
    code(DIVINT, 0) \
    code(MODINT, 0) \
    code(ANDINT, 0) \
    code(ORINT, 0) \
    code(XORINT, 0) \
    code(LSLINT, 0) \
    code(LSRINT, 0) \
    code(ASRINT, 0) \
    code(EQ, 0) \
    code(NEQ, 0) \
    code(LTINT, 0) \
    code(LEINT, 0) \
    code(GTINT, 0) \
    code(GEINT, 0) \
    code(OFFSETINT, 1) \
    code(OFFSETREF, 1) \
    code(ISINT, 0) \
    code(GETMETHOD, 0) \
    code(BEQ, 2) \
    code(BNEQ, 2) \
    code(BLTINT, 2) \
    code(BLEINT, 2) \
    code(BGTINT, 2) \
    code(BGEINT, 2) \
    code(ULTINT, 0) \
    code(UGEINT, 0) \
    code(BULTINT, 2) \
    code(BUGEINT, 2) \
    code(GETPUBMET, 2) \
    code(GETDYNMET, 0) \
    code(STOP, 0) \
    code(EVENT, 0) \
    code(BREAK, 0)

/**
 * Enum of all the opcodes
 * Will help code readability when switching on opcode type
 */
enum OpCodes {
    #define DEFINE_ENUM(type, arity) \
        type,
    OPCODE_NODE_LIST(DEFINE_ENUM)
    #undef DEFINE_ENUM
};

/**
 * Structure of a basic opcode, 
 * containing it's name and arity (number of arguments)
 */
struct OpCode {
    const char Name[32];
    unsigned short Arity;
};

/**
 * Array containing every opcode's name and arity
 */
static OpCode AllOpCodes[146] = {
    #define DEFINE_OPCODES(type, arity) \
        {#type, arity},
    OPCODE_NODE_LIST(DEFINE_OPCODES)
    #undef DEFINE_OPCODES

};

enum InstrAnnotation {
    NOTHING, FUNCTION_START, FUNCTION_RETURN, BLOCK_START
};

std::map<int, int>& getCodeOffsetArgs();

/**
 * Struct representing a concrete instruction in a bytecode file
 */
struct ZInstruction {
public:
    uint32_t OpNum;
    int32_t Args[4];
    int32_t idx;
    int32_t ClosureRecFns[32];
    uint32_t Annotation;

    inline unsigned short Arity() {
        return AllOpCodes[OpNum].Arity;
    }

    inline const char* Name() {
        return AllOpCodes[OpNum].Name;
    }

    inline void Print(bool LineNums = false) {
        if (LineNums) std::cout << idx << ": ";
        std::cout << Name() << " ";
        for (int i = 0; i < Arity(); i++) {
            std::cout << Args[i] << " ";
        }
        if (this->isClosureRec()) {
            for (int i = 0; i < Args[0]; i++) {
                std::cout << this->ClosureRecFns[i] << " ";
            }
        }
        std::cout << std::endl;
    }

    inline bool isJumpInst() {
        switch (OpNum) {
            case BRANCH:
            case BRANCHIF:
            case BRANCHIFNOT:
            case BEQ:
            case BNEQ:
            case BLTINT:
            case BLEINT:
            case BGTINT:
            case BGEINT:
            case BULTINT:
            case BUGEINT:
                return true;
            default:
                return false;
        }
    }

    inline bool isCondJump() {
        return isJumpInst() && OpNum != BRANCH;
    }

    inline bool isUncondJump() {
        return OpNum == BRANCH;
    }

    inline bool isClosure() {
        return OpNum == CLOSURE;
    }

    inline bool isClosureRec() {
        return OpNum == CLOSUREREC;
    }

    inline bool isReturn() {
        switch (OpNum) {
            case RAISE:
            case APPTERM:
            case APPTERM1:
            case APPTERM2:
            case APPTERM3:
            case STOP:
            case RETURN:
                return true;
            default:
                return false;
        }
    }

    inline bool hasCodeOffset() {
        auto CodeOffsetArgs = getCodeOffsetArgs();
        bool hasCodeOff = (CodeOffsetArgs.find(OpNum) != CodeOffsetArgs.end());
        return hasCodeOff;
    }

    inline bool getCodeOffsetArgIdx() {
        auto CodeOffsetArgs = getCodeOffsetArgs();
        return CodeOffsetArgs.find(this->OpNum)->second;
    }

    inline int getDestIdx() {
        auto JumpInsts = getCodeOffsetArgs();
        return Args[JumpInsts[OpNum]];
    }

};

void readInstructions(std::vector<ZInstruction*>& Instructions, int32_t* TabInst, uint32_t Size);

inline void printInstructions(std::vector<ZInstruction*>& Instructions, bool LineNums=true, int StartAt=0) {
    for (uint32_t i = StartAt; i < Instructions.size(); i++) {

        switch (Instructions[i]->Annotation) {
            case FUNCTION_START:
                std::cout << " ===== FUNCTION ===== " << std::endl;
                break;
            case BLOCK_START:
                std::cout << " ===== BLOCK ===== " << std::endl;
                break;
        }

        Instructions[i]->Print(LineNums);

        if (Instructions[i]->Annotation == FUNCTION_RETURN) 
            std::cout << std::endl;
    }
}

void annotateNodes(std::vector<ZInstruction*>& Instructions);


#endif
