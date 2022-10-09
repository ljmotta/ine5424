#include <architecture/mmu.h>
#include <system.h>

extern "C" char _edata;
extern "C" char __bss_start;
extern "C" char _end;

__BEGIN_SYS

void MMU::init()
{
    db<Init, MMU>(TRC) << "MMU::init()" << endl;
    db<Init, MMU>(INF) << "MMU::init::dat.e=" << &_edata << ",bss.b=" << &__bss_start << ",bss.e=" << &_end << endl;
    // free FREE_BASE, (FREE_TOP - FREE_BASE)
    free(Traits<Machine>::FREE_BASE, pages(Traits<Machine>::FREE_TOP - Traits<Machine>::FREE_BASE));

    // free [_end, (PAGE_TABLES - _end)]
    // free(align_page(&_end), pages(Traits<Machine>::PAGE_TABLE - align_page(&_end)));

    // free [BOOT_STACK, STACK_SIZE]
    // free(Memory_Map::BOOT_STACK, pages(Traits<Machine>::STACK_SIZE));

}

__END_SYS
