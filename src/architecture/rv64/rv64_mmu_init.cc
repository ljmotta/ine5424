#include <architecture/mmu.h>
#include <system.h>

extern "C" char _edata;
extern "C" char __bss_start;
extern "C" char _end;

__BEGIN_SYS

void Sv39_MMU::init()
{
    db<Init, MMU>(TRC) << "Sv39_MMU::init()" << endl;
    db<Init, MMU>(INF) << "Sv39_MMU::init::dat.e=" << &_edata << ",bss.b=" << &__bss_start << ",bss.e=" << &_end << endl;

    // free page_table
    // free(Memory_Map::RAM_BASE, pages(Memory_Map::RAM_BASE + (512 * 0x200000) - Memory_Map::RAM_BASE));

    // free stack
    free(Memory_Map::RAM_TOP + 1 - Traits<Machine>::STACK_SIZE, pages(Traits<Machine>::STACK_SIZE));
}

__END_SYS
