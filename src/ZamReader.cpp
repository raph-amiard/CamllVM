#include <ZamReader.hpp>
#include <EndianUtils.hpp>

using namespace std;

struct FileSection {
    char Name[5];
    uint32_t Size;
};

void readLittleEndian(ifstream& FileStream, uint32_t& Dest) {
    read(FileStream, Dest);
    Dest = toLittleEndian(Dest);
}

void readBigEndian(ifstream& FileStream, uint32_t& Dest) {
    read(FileStream, Dest);
    Dest = toBigEndian(Dest);
}

ZamFile::ZamFile(const char* Filename) {

    // Open compiled ocaml file
    ifstream FileStream(Filename, ifstream::in);
    FileStream.seekg(-MAGIC_STRING_SIZE, ios::end);

    // Verify magic string
    char MagicBuffer[MAGIC_STRING_SIZE + 1] = {0};
    string MagicString(MAGIC_STRING);
    FileStream.read(MagicBuffer, MAGIC_STRING_SIZE);
    if (MagicString.compare(MagicBuffer) != 0) {
        throw logic_error("Not a valid compiled ocaml file !");
    }

    // Get number of sections
    uint32_t NbSections;
    FileStream.seekg(-MAGIC_STRING_SIZE - 4, ios::end);
    readLittleEndian(FileStream, NbSections);
    cout << NbSections << endl;

    // Get sections names and data
    list<FileSection> SectionsList;
    int Pos = -MAGIC_STRING_SIZE - 12;

    for (;NbSections > 0; NbSections--) {
        FileSection s = {{0}, 0};
        FileStream.seekg(Pos, ios::end);
        FileStream.read(s.Name, 4);
        readLittleEndian(FileStream, s.Size);
        Pos -= 8;
        SectionsList.push_front(s);
    }

    Pos = 20;

    for (FileSection fs : SectionsList) {
        FileStream.seekg(Pos, ios::beg);
        if (string("CODE") == fs.Name) {
            readInstructions(FileStream, fs.Size);
        } else if (string("DATA") == fs.Name) {
            cout << "DATA " << Pos << endl;
        }
        Pos += fs.Size;
    }

    printInstructions();

}

void ZamFile::readInstructions(ifstream& FileStream, uint32_t Size) {
    Instruction Inst;
    while (Size > 0) {
        // Read instruction
        // And then fill arguments values
        readBigEndian(FileStream, Inst.OpNum);
        for (int i = 0; i < Inst.Arity(); i++) {
            readBigEndian(FileStream, Inst.Args[i]);
        }
        Size -= 4 + (4 * Inst.Arity());
        Instructions.push_back(Inst);
    }
}
