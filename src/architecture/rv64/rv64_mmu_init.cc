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
    unsigned long qtt_of_pages = pages(Memory_Map::FREE_TOP - Memory_Map::FREE_BASE);
    db<Init, MMU>(INF) << "MMU::init::qtt_of_pages=" << qtt_of_pages << ",FREE_BASE=" << hex << Memory_Map::FREE_BASE  << endl;

    free(Memory_Map::FREE_BASE, qtt_of_pages);
}

__END_SYS
