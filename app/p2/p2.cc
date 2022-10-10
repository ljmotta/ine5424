#include <utility/ostream.h>
#include <process.h>
#include <memory.h>
#include <architecture/rv64/rv64_mmu.h>

using namespace EPOS;

OStream cout;

int main()
{
    cout << "Hello World" << endl;

    unsigned long *data_base = reinterpret_cast<unsigned long*>(Memory_Map::APP_DATA);
    Segment *code_seg1 = new Segment(1024*4, MMU::Flags::SYS);
    cout << "Data base=" << *data_base << endl;
    cout << "Code seg=" << reinterpret_cast<unsigned long*>(code_seg1) << endl;

    return 0;
}
