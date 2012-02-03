#include <ZamReader.hpp>
#include <EndianUtils.hpp>

using namespace std;

struct FileSection {
    char Name[5];
    uint32_t Size;
};

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
