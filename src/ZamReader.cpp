#include <ZamReader.hpp>

using namespace std;

struct FileSection {
    char Name[5];
    uint32_t Size;
};

bool IsBigEndian() {
    union {
        uint32_t i;
        char c[4];
    } Bint = {0x01020304};
    return Bint.c[0] == 1;
}

uint32_t SwapEndianness(uint32_t in) {
    unsigned char b1, b2, b3, b4;

    b1 = in & 255;
    b2 = ( in >> 8 ) & 255;
    b3 = ( in >> 16 ) & 255;
    b4 = ( in >> 24 ) & 255;

    return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

uint32_t ToLittleEndian(uint32_t in) {
    return IsBigEndian() ? in : SwapEndianness(in);
}

void ReadLittleEndian(ifstream& FileStream, uint32_t& Dest) {
    Read(FileStream, Dest);
    Dest = ToLittleEndian(Dest);
}

ZamFile::ZamFile(const char* Filename) {

    // Open compiled ocaml file
    ifstream FileStream(Filename, ifstream::in);
    FileStream.seekg(-12, ios::end);

    // Verify magic string
    char MagicBuffer[13] = {0};
    string MagicString("Caml1999X008");
    FileStream.read(MagicBuffer, 12);
    if (MagicString.compare(MagicBuffer) != 0) {
        throw logic_error("Not a valid compiled ocaml file !");
    }

    // Get number of sections
    uint32_t NbSections;
    FileStream.seekg(-16, ios::end);
    ReadLittleEndian(FileStream, NbSections);
    cout << NbSections << endl;

    // Get sections names and data
    list<FileSection> SectionsList;
    int Pos = -24;

    for (;NbSections > 0; NbSections--) {
        FileSection s = {{0}, 0};
        FileStream.seekg(Pos, ios::end);
        FileStream.read(s.Name, 4);
        ReadLittleEndian(FileStream, s.Size);
        Pos -= 8;
        SectionsList.push_front(s);
    }

    Pos = 20;

    for (FileSection fs : SectionsList) {
        FileStream.seekg(Pos, ios::beg);
        if (string("CODE") == fs.Name) {
            ReadInstructions(FileStream, fs.Size);
        } else if (string("DATA") == fs.Name) {
            cout << "DATA " << Pos << endl;
        }
        Pos += fs.Size;
    }

    PrintInstructions();

}

void ZamFile::ReadInstructions(ifstream& FileStream, uint32_t Size) {
    Instruction Inst;
    while (Size > 0) {
        // Read instruction
        // And then fill arguments values
        Read(FileStream, Inst.OpNum);
        for (int i = 0; i < Inst.Arity(); i++) {
            Read(FileStream, Inst.Args[i]);
        }
        Size -= 4 + (4 * Inst.Arity());
        Instructions.push_back(Inst);
    }
}
