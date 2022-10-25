#include <architecture/mmu.h>
#include <system.h>

extern "C" char _edata;
extern "C" char __bss_start;
extern "C" char _end;

__BEGIN_SYS

void Sv39_MMU::init()
{
    db<Init, MMU>(TRC) << "MMU::init()" << endl;
    db<Init, MMU>(INF) << "MMU::init::dat.e=" << &_edata << ",bss.b=" << &__bss_start << ",bss.e=" << &_end << endl;

    // free all memory
    free(Memory_Map::FREE_BASE, pages(Memory_Map::FREE_TOP - Memory_Map::FREE_BASE));
}

__END_SYS
