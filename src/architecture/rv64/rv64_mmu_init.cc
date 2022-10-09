#include <architecture/mmu.h>
#include <system.h>

extern "C" char _edata;
extern "C" char __bss_start;
extern "C" char _end;

__BEGIN_SYS

void Sv39_MMU::init()
{
    // db<Init, MMU>(TRC) << "MMU::init()" << endl;
    // free(System::info()->pmm.free1_base, pages(System::info()->pmm.free1_top - System::info()->pmm.free1_base));

    db<Init, MMU>(TRC) << "Sv39_MMU::init()" << endl;
    db<Init, MMU>(INF) << "Sv39_MMU::init::dat.e=" << &_edata << ",bss.b=" << &__bss_start << ",bss.e=" << &_end << endl;

    // For machines that do not feature a real MMU, frame size = 1 byte
    // Allocations (using Grouping_List<Frame>::search_decrementing() start from the end
    // To preserve the BOOT stacks until the end of INIT, the free memory list initialization is split in two sections
    // with allocations (from the end) of the first section taking place first
    free(align_page(&_end), pages(Traits<Machine>::PAGE_TABLES - align_page(&_end))); // [align_page(&_end), 0x87bfc000]
    free(Memory_Map::RAM_TOP + 1 - Traits<Machine>::STACK_SIZE * Traits<Machine>::CPUS, pages(Traits<Machine>::STACK_SIZE * Traits<Machine>::CPUS));

}

__END_SYS
