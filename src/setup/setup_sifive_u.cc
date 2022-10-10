// EPOS SiFive-U (RISC-V) SETUP

#include <architecture.h>
#include <machine.h>
#include <utility/elf.h>
#include <utility/string.h>

extern "C" {
    void _start();

    void _int_entry();

    // SETUP entry point is in .init (and not in .text), so it will be linked first and will be the first function after the ELF header in the image
    void _entry() __attribute__ ((used, naked, section(".init")));
    void _setup();

    // LD eliminates this variable while performing garbage collection, that's why the used attribute.
    char __boot_time_system_info[sizeof(EPOS::S::System_Info)] __attribute__ ((used)) = "<System_Info placeholder>"; // actual System_Info will be added by mkbi!
}

__BEGIN_SYS

extern OStream kout, kerr;

class Setup
{
private:
    // Physical memory map
    static const unsigned long RAM_BASE         = Memory_Map::RAM_BASE;
    static const unsigned long RAM_TOP          = Memory_Map::RAM_TOP;
    static const unsigned long MIO_BASE         = Memory_Map::MIO_BASE;
    static const unsigned long MIO_TOP          = Memory_Map::MIO_TOP;
    static const unsigned long FREE_BASE        = Memory_Map::FREE_BASE;
    static const unsigned long FREE_TOP         = Memory_Map::FREE_TOP;
    static const unsigned long SETUP            = Memory_Map::SETUP;
    static const unsigned long BOOT_STACK       = Memory_Map::BOOT_STACK;

    // Architecture Imports
    typedef CPU::Reg Reg;
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::Log_Addr Log_Addr;
    typedef MMU::RV64_Flags RV64_Flags;
    typedef MMU::Page_Table Page_Table;
    typedef MMU::Page_Directory Page_Directory;
    typedef MMU::PT_Entry PT_Entry;

public:
    Setup();

private:
    void say_hi();
    void start_mmu();
    void call_next();

private:
    System_Info * si;
};


Setup::Setup()
{
    Display::init();
    kout << endl;
    kerr << endl;
    
    si = reinterpret_cast<System_Info *>(&__boot_time_system_info);
    if(si->bm.n_cpus > Traits<Machine>::CPUS)
        si->bm.n_cpus = Traits<Machine>::CPUS;

    db<Setup>(TRC) << "Setup(si=" << reinterpret_cast<void *>(si) << ",sp=" << CPU::sp() << ")" << endl;
    db<Setup>(INF) << "Setup:si=" << *si << endl;

    // Print basic facts about this EPOS instance
    say_hi();

    // Setup MMU
    start_mmu();

    // SETUP ends here, so let's transfer control to the next stage (INIT or APP)
    call_next();
}


void Setup::say_hi()
{
    db<Setup>(TRC) << "Setup::say_hi()" << endl;
    db<Setup>(INF) << "System_Info=" << *si << endl;

    if(si->bm.application_offset == -1U)
        db<Setup>(ERR) << "No APPLICATION in boot image, you don't need EPOS!" << endl;

    kout << "This is EPOS!\n" << endl;
    kout << "Setting up this machine as follows: " << endl;
    kout << "  Mode:         " << ((Traits<Build>::MODE == Traits<Build>::LIBRARY) ? "library" : (Traits<Build>::MODE == Traits<Build>::BUILTIN) ? "built-in" : "kernel") << endl;
    kout << "  Processor:    " << Traits<Machine>::CPUS << " x RV" << Traits<CPU>::WORD_SIZE << " at " << Traits<CPU>::CLOCK / 1000000 << " MHz (BUS clock = " << Traits<CPU>::CLOCK / 1000000 << " MHz)" << endl;
    kout << "  Machine:      SiFive-U" << endl;
    kout << "  Memory:       " << (RAM_TOP + 1 - RAM_BASE) / (1024*1024) << " MB [" << reinterpret_cast<void *>(RAM_BASE) << ":" << reinterpret_cast<void *>(RAM_TOP) << "]" << endl;
    kout << "  User memory:  " << (FREE_TOP - FREE_BASE) / (1024*1024) << " MB [" << reinterpret_cast<void *>(FREE_BASE) << ":" << reinterpret_cast<void *>(FREE_TOP) << "]" << endl;
    kout << "  I/O space:    " << (MIO_TOP + 1 - MIO_BASE) / (1024*1024) << " MB [" << reinterpret_cast<void *>(MIO_BASE) << ":" << reinterpret_cast<void *>(MIO_TOP) << "]" << endl;
    kout << "  Node Id:      ";
    if(si->bm.node_id != -1)
        kout << si->bm.node_id << " (" << Traits<Build>::NODES << ")" << endl;
    else
        kout << "will get from the network!" << endl;
    kout << "  Position:     ";
    if(si->bm.space_x != -1)
        kout << "(" << si->bm.space_x << "," << si->bm.space_y << "," << si->bm.space_z << ")" << endl;
    else
        kout << "will get from the network!" << endl;
    if(si->bm.extras_offset != -1UL)
        kout << "  Extras:       " << si->lm.app_extra_size << " bytes" << endl;

    kout << endl;
}

typedef unsigned long Reg;

void Setup::start_mmu() {
    // create _master under the PAGE_TABLE address
    Page_Directory *_master = MMU::current();
    Reg pd = Traits<Machine>::PAGE_TABLE;
    _master = new ((void *)pd) Page_Directory();

    // qtt of pages for (RAM_TOP + 1) - RAM_BASE
    unsigned pages = MMU::pages(Traits<Machine>::RAM_TOP + 1 - Traits<Machine>::RAM_BASE);
    unsigned entries = MMU::page_tables(pages);
    _master->remap(pd, 0, entries, RV64_Flags::V);

    // Activate MMU here with satp MODE = 1000
    // CPU::satp((1UL << 63) | (Traits<Machine>::PAGE_TABLE >> 12));
}

void Setup::call_next()
{
    db<Setup>(INF) << "SETUP ends here!" << endl;

    // Call the next stage
    static_cast<void (*)()>(_start)();

    // SETUP is now part of the free memory and this point should never be reached, but, just in case ... :-)
    db<Setup>(ERR) << "OS failed to init!" << endl;
}

__END_SYS

using namespace EPOS::S;

void _entry() // machine mode
{
    if(CPU::mhartid() != 0)                             // SiFive-U requires 2 cores, so we disable core 1 here
        CPU::halt();

    CPU::mstatusc(CPU::MIE);                            // disable interrupts (they will be reenabled at Init_End)
    CPU::mies(CPU::MSI);                                // enable interrupts generation by CLINT
    CLINT::mtvec(CLINT::DIRECT, _int_entry);            // setup a preliminary machine mode interrupt handler pointing it to _int_entry

    CPU::sp(Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE - sizeof(long)); // set the stack pointer, thus creating a stack for SETUP

    Machine::clear_bss();
    // CPU::satp(0);

    CPU::mstatus(CPU::MPP_M);                           // stay in machine mode at mret

    CPU::mepc(CPU::Reg(&_setup));                       // entry = _setup
    CPU::mret();                                        // enter supervisor mode at setup (mepc) with interrupts enabled (mstatus.mpie = true)
}

void _setup() // supervisor mode
{
    kerr  << endl;
    kout  << endl;

    Setup setup;
}
