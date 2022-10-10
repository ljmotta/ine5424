#include <utility/ostream.h>
#include <process.h>
#include <memory.h>
#include <architecture/rv64/rv64_mmu.h>

using namespace EPOS;

OStream cout;

int main()
{
    cout << "Hello World" << endl;

    unsigned * data_base = reinterpret_cast<unsigned*>(Memory_Map::APP_DATA);
    Segment * code_seg1 = new Segment(1024*4, MMU::Flags::SYS);
    Segment * data_seg1 = new Segment(1024*4, MMU::Flags::SYS);
    cout << "Create Task1" << endl;
    cout << "Data base=" << *data_base << endl;
    cout << "code seg=" << reinterpret_cast<unsigned*>(code_seg1) << endl;
    cout << "data seg=" << reinterpret_cast<unsigned*>(data_seg1) << endl;

    return 0;
}
