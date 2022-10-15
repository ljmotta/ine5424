// EPOS SiFive-U (RISC-V) SETUP

// #include <architecture.h>
// #include <machine.h>
// #include <utility/elf.h>
// #include <utility/string.h>

// extern "C" {
//     void _start();

//     void _int_entry();

//     // SETUP entry point is in .init (and not in .text), so it will be linked first and will be the first function after the ELF header in the image
//     void _entry() __attribute__ ((used, naked, section(".init")));
//     void _setup();

//     // LD eliminates this variable while performing garbage collection, that's why the used attribute.
//     char __boot_time_system_info[sizeof(EPOS::S::System_Info)] __attribute__ ((used)) = "<System_Info placeholder>"; // actual System_Info will be added by mkbi!
// }

// __BEGIN_SYS

// extern OStream kout, kerr;

// class Setup
// {
// private:
//     // Physical memory map
//     static const unsigned long RAM_BASE         = Memory_Map::RAM_BASE;
//     static const unsigned long RAM_TOP          = Memory_Map::RAM_TOP;
//     static const unsigned long MIO_BASE         = Memory_Map::MIO_BASE;
//     static const unsigned long MIO_TOP          = Memory_Map::MIO_TOP;
//     static const unsigned long FREE_BASE        = Memory_Map::FREE_BASE;
//     static const unsigned long FREE_TOP         = Memory_Map::FREE_TOP;
//     static const unsigned long SETUP            = Memory_Map::SETUP;
//     static const unsigned long BOOT_STACK       = Memory_Map::BOOT_STACK;

//     // Architecture Imports
//     typedef CPU::Reg Reg;
//     typedef CPU::Phy_Addr Phy_Addr;
//     typedef CPU::Log_Addr Log_Addr;
//     typedef MMU::RV64_Flags RV64_Flags;
//     typedef MMU::Page_Table Page_Table;
//     typedef MMU::Page_Directory Page_Directory;
//     typedef MMU::PT_Entry PT_Entry;

// public:
//     Setup();

// private:
//     void say_hi();
//     void start_mmu();
//     void call_next();

// private:
//     System_Info * si;
// };


// Setup::Setup()
// {
//     Display::init();
//     kout << endl;
//     kerr << endl;
    
//     si = reinterpret_cast<System_Info *>(&__boot_time_system_info);
//     if(si->bm.n_cpus > Traits<Machine>::CPUS)
//         si->bm.n_cpus = Traits<Machine>::CPUS;

//     db<Setup>(TRC) << "Setup(si=" << reinterpret_cast<void *>(si) << ",sp=" << CPU::sp() << ")" << endl;
//     db<Setup>(INF) << "Setup:si=" << *si << endl;

//     // Print basic facts about this EPOS instance
//     say_hi();

//     // Setup MMU
//     start_mmu();

//     // SETUP ends here, so let's transfer control to the next stage (INIT or APP)
//     call_next();
// }


// void Setup::say_hi()
// {
//     db<Setup>(TRC) << "Setup::say_hi()" << endl;
//     db<Setup>(INF) << "System_Info=" << *si << endl;

//     if(si->bm.application_offset == -1U)
//         db<Setup>(ERR) << "No APPLICATION in boot image, you don't need EPOS!" << endl;

//     kout << "This is EPOS!\n" << endl;
//     kout << "Setting up this machine as follows: " << endl;
//     kout << "  Mode:         " << ((Traits<Build>::MODE == Traits<Build>::LIBRARY) ? "library" : (Traits<Build>::MODE == Traits<Build>::BUILTIN) ? "built-in" : "kernel") << endl;
//     kout << "  Processor:    " << Traits<Machine>::CPUS << " x RV" << Traits<CPU>::WORD_SIZE << " at " << Traits<CPU>::CLOCK / 1000000 << " MHz (BUS clock = " << Traits<CPU>::CLOCK / 1000000 << " MHz)" << endl;
//     kout << "  Machine:      SiFive-U" << endl;
//     kout << "  Memory:       " << (RAM_TOP + 1 - RAM_BASE) / (1024*1024) << " MB [" << reinterpret_cast<void *>(RAM_BASE) << ":" << reinterpret_cast<void *>(RAM_TOP) << "]" << endl;
//     kout << "  User memory:  " << (FREE_TOP - FREE_BASE) / (1024*1024) << " MB [" << reinterpret_cast<void *>(FREE_BASE) << ":" << reinterpret_cast<void *>(FREE_TOP) << "]" << endl;
//     kout << "  I/O space:    " << (MIO_TOP + 1 - MIO_BASE) / (1024*1024) << " MB [" << reinterpret_cast<void *>(MIO_BASE) << ":" << reinterpret_cast<void *>(MIO_TOP) << "]" << endl;
//     kout << "  Node Id:      ";
//     if(si->bm.node_id != -1)
//         kout << si->bm.node_id << " (" << Traits<Build>::NODES << ")" << endl;
//     else
//         kout << "will get from the network!" << endl;
//     kout << "  Position:     ";
//     if(si->bm.space_x != -1)
//         kout << "(" << si->bm.space_x << "," << si->bm.space_y << "," << si->bm.space_z << ")" << endl;
//     else
//         kout << "will get from the network!" << endl;
//     if(si->bm.extras_offset != -1UL)
//         kout << "  Extras:       " << si->lm.app_extra_size << " bytes" << endl;

//     kout << endl;
// }

// void Setup::start_mmu() {
//     // create _master under the PAGE_TABLE address
//     // Page_Directory *_master = MMU::current();
//     // unsigned long pd = Traits<Machine>::PAGE_TABLE;
//     // _master = new ((void *)pd) Page_Directory();

//     // qtt of pages for (RAM_TOP + 1) - RAM_BASE
//     // unsigned pages = MMU::pages(Traits<Machine>::RAM_TOP + 1 - Traits<Machine>::RAM_BASE);
//     // unsigned entries = MMU::page_tables(pages);
//     // _master->remap(pd, 0, entries, RV64_Flags::V);

//     // Activate MMU here with satp MODE = 1000
//     // CPU::satp((1UL << 63) | (Traits<Machine>::PAGE_TABLE >> 12));
// }

// void Setup::call_next()
// {
//     db<Setup>(INF) << "SETUP ends here!" << endl;

//     // Call the next stage
//     kout << "start" << endl;
//     static_cast<void (*)()>(_start)();

//     // SETUP is now part of the free memory and this point should never be reached, but, just in case ... :-)
//     db<Setup>(ERR) << "OS failed to init!" << endl;
// }

// __END_SYS

// using namespace EPOS::S;

// void _entry() // machine mode
// {
    // if(CPU::mhartid() != 1)                             // SiFive-U requires 2 cores, so we disable core 0 here, don't have MMU
    //     CPU::halt();
    
    // CPU::satp_zero();
    // Machine::clear_bss();

    // CPU::int_disable();                            // disable interrupts (they will be reenabled at Init_End)
    // CPU::mies(CPU::MSI);                           // enable interrupts generation by CLINT
    // CLINT::mtvec(CLINT::DIRECT, _int_entry);       // setup a preliminary machine mode interrupt handler pointing it to _int_entry

    // CPU::sp(Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE - sizeof(long));

    // // delegate Software, Timer and External interrupts to Supervisor
    // CPU::mideleg(CPU::SSI | CPU::STI | CPU::SEI);
    // // delegate All exceptions to Supervisor
    // CPU::medeleg(0xff);

    // CPU::mstatus(CPU::MPP_S | CPU::MPIE); // change to supervirsor

    // // --- necessary ???
    // // Set up the Physical Memory Protection registers correctly
    // ASM("    li t4, 31              \n" 
    //     "    csrw pmpcfg0, t4       \n"
    //     "    li t5, (1 << 55) - 1   \n"
    //     "    csrw pmpaddr0, t5      \n");
    // // --- ???

    // CPU::mepc(CPU::Reg(&_setup));                       // set the address of the function that will run in supervisor
    // CPU::mret();                                        // enter supervisor mode
// }

// void _setup() // supervisor mode
// {
//     kerr  << endl;
//     kout  << endl;

//     Setup setup;
// }


// // EPOS SiFive-U (RISC-V) SETUP

// #include <architecture.h>
// #include <machine.h>
// #include <utility/elf.h>
// #include <utility/string.h>

// // EPOS RISC-V sifive SETUP

// using namespace EPOS::S;
// typedef unsigned long Reg;

// extern "C"
// {
//     [[gnu::naked, gnu::section(".init")]] void _setup();
//     void _int_entry();
//     void _start();
//     void _wait() {
//         CPU::halt();
//         _start();
//     }
//     char __boot_time_system_info[sizeof(EPOS::S::System_Info)] __attribute__ ((used)) = "<System_Info placeholder>"; // actual System_Info will be added by mkbi!
// }

// extern "C" [[gnu::interrupt, gnu::aligned(8)]] void _mmode_forward() {
//     Reg id = CPU::mcause();
//     if((id & CLINT::INT_MASK) == CLINT::IRQ_MAC_TIMER) {
//         Timer::reset();
//         CPU::sie(CPU::STI);
//     }
//     Reg interrupt_id = 1 << ((id & CLINT::INT_MASK) - 2);
//     if(CPU::int_enabled() && (CPU::sie() & (interrupt_id)))
//         CPU::mip(interrupt_id);
// }

// __BEGIN_SYS

// extern OStream kout, kerr;

// class Setup_Sifive_U {
// private:
//     typedef CPU::Reg Reg;
//     typedef MMU::RV64_Flags RV64_Flags;
//     typedef MMU::Page_Table Page_Table;
//     typedef MMU::Page_Directory Page_Directory;
//     typedef MMU::PT_Entry PT_Entry;

//     static const unsigned long RAM_BASE         = Memory_Map::RAM_BASE;
//     static const unsigned long RAM_TOP          = Memory_Map::RAM_TOP;
//     static const unsigned long MIO_BASE         = Memory_Map::MIO_BASE;
//     static const unsigned long MIO_TOP          = Memory_Map::MIO_TOP;
//     static const unsigned long FREE_BASE        = Memory_Map::FREE_BASE;
//     static const unsigned long FREE_TOP         = Memory_Map::FREE_TOP;
//     static const unsigned long SETUP            = Memory_Map::SETUP;
//     static const unsigned long BOOT_STACK       = Memory_Map::BOOT_STACK;

// public:
//     static void init() { setup_machine_environment(); }
//     static void setup_machine_environment();
//     static void setup_supervisor_environment();
//     static void build_page_tables();
//     static void clean_bss();
//     static void say_hi();

// // private:
// //     System_Info * si;
// };

// void Setup_Sifive_U::build_page_tables()
// {
//     // Address of the Directory
//     Reg page_tables = Traits<Machine>::PAGE_TABLE;
//     auto _master = MMU::current();
//     kout << "get current _master" << endl;
//     _master = new ((void *) page_tables) Page_Directory();

//     // Number of kernel entries in each directory
//     kout << "set sys_entries" << endl;
//     unsigned long sys_entries = 512 + MMU::page_tables(MMU::pages(Traits<Machine>::RAM_TOP + 1 - Traits<Machine>::RAM_BASE));

//     kout << "remap" << endl;
//     _master->remap(page_tables + 4096, 0, sys_entries, MMU::RV64_Flags::V);

//     // Map logical addrs back to themselves; with this, the kernel may access any
//     // physical RAM address directly (as if paging wasn't there)
//     kout << "create table" << endl;
//     for(unsigned long i = 0; i < sys_entries; i++) {
//         Page_Table * pt = new ( (void *)(page_tables + 4*512*(i+1)) ) Page_Table();
//         pt->remap(i * 1024*4096, 0, 512, MMU::RV64_Flags::SYS);
//     }
//     kout << "build page talbes finished" << endl;
// }

// void Setup_Sifive_U::say_hi()
// {
//     db<Setup_Sifive_U>(TRC) << "Setup::say_hi()" << endl;
//     // db<Setup_Sifive_U>(INF) << "System_Info=" << *si << endl;

//     // if(si->bm.application_offset == -1U)
//     //     db<Setup_Sifive_U>(ERR) << "No APPLICATION in boot image, you don't need EPOS!" << endl;

//     kout << "This is EPOS!\n" << endl;
//     kout << "Setting up this machine as follows: " << endl;
//     kout << "  Mode:         " << ((Traits<Build>::MODE == Traits<Build>::LIBRARY) ? "library" : (Traits<Build>::MODE == Traits<Build>::BUILTIN) ? "built-in" : "kernel") << endl;
//     kout << "  Processor:    " << Traits<Machine>::CPUS << " x RV" << Traits<CPU>::WORD_SIZE << " at " << Traits<CPU>::CLOCK / 1000000 << " MHz (BUS clock = " << Traits<CPU>::CLOCK / 1000000 << " MHz)" << endl;
//     kout << "  Machine:      SiFive-U" << endl;
//     kout << "  Memory:       " << (RAM_TOP + 1 - RAM_BASE) / (1024*1024) << " MB [" << reinterpret_cast<void *>(RAM_BASE) << ":" << reinterpret_cast<void *>(RAM_TOP) << "]" << endl;
//     kout << "  User memory:  " << (FREE_TOP - FREE_BASE) / (1024*1024) << " MB [" << reinterpret_cast<void *>(FREE_BASE) << ":" << reinterpret_cast<void *>(FREE_TOP) << "]" << endl;
//     kout << "  I/O space:    " << (MIO_TOP + 1 - MIO_BASE) / (1024*1024) << " MB [" << reinterpret_cast<void *>(MIO_BASE) << ":" << reinterpret_cast<void *>(MIO_TOP) << "]" << endl;
//     kout << "  Node Id:      ";
//     // if(si->bm.node_id != -1)
//     //     kout << si->bm.node_id << " (" << Traits<Build>::NODES << ")" << endl;
//     // else
//     //     kout << "will get from the network!" << endl;
//     // kout << "  Position:     ";
//     // if(si->bm.space_x != -1)
//     //     kout << "(" << si->bm.space_x << "," << si->bm.space_y << "," << si->bm.space_z << ")" << endl;
//     // else
//     //     kout << "will get from the network!" << endl;
//     // if(si->bm.extras_offset != -1UL)
//     //     kout << "  Extras:       " << si->lm.app_extra_size << " bytes" << endl;

//     kout << endl;
// }

// void Setup_Sifive_U::setup_supervisor_environment()
// {
//     CPU::stvec(CPU::Reg(&_int_entry) & 0xfffffffc);

//     say_hi();

//     // We must clean the bss before setting MMU::_master
//     clean_bss();

//     // This creates and configures the kernel page tables (which map logical==physical)
//     build_page_tables();

//     // forward everything
//     CPU::satp((1UL << 63) | (Traits<Machine>::PAGE_TABLE >> 12));
//     CPU::sepc(CPU::Reg(&_start));

//     // Interrupts will remain disable until the Context::load at Init_First
//     CPU::sstatus(CPU::SPP_S);
//     CPU::sie(CPU::SSI | CPU::STI | CPU::SEI);

//     ASM("sret");
// }

// void Setup_Sifive_U::setup_machine_environment()
// {
//     // Core doesn't have MMU
//     if(CPU::mhartid() == 0) {
//         CPU::halt();
//     }
//     // We first configure the M-mode CSRs and then switch to S-mode
//     // configure paging. After that, we won't return to M-mode; an exception
//     // is the forwarding of ints and excps to S-mode.

//     CPU::mie(CPU::MSI | CPU::MTI | CPU::MEI);
//     CPU::mint_disable();

//     // Set stack for each core
//     CPU::sp(Memory_Map::BOOT_STACK);

//     Reg core = CPU::mhartid();
//     CPU::tp(core);

//     CPU::satp_zero();
    
//     // Forward all ints and excs to S-mode.
//     //!ECALLS: Not yet implemented.
//     CPU::mideleg(CPU::SSI | CPU::STI | CPU::SEI);
//     CPU::medeleg(0xffff);

//     // We need to set:
//     //      MPP_S: to switch to S-mode after mret
//     //      MPIE:  otherwise we won't ever receive interrupts
//     CPU::mstatus(CPU::MPP_S | CPU::MPIE);

//     // Set up the PMP registers correctly
//     ASM("    li t4, 31              \n" 
//         "    csrw pmpcfg0, t4       \n"
//         "    li t5, (1 << 55) - 1   \n"
//         "    csrw pmpaddr0, t5      \n");

//     // All ints received in M-mode are forwarded to S-mode.
//     // The first two bits indicate the mode: Direct or Vectored;
//     // we opted for Direct.
//     CPU::mtvec(CPU::Reg(&_mmode_forward) & 0xfffffffc);
//     CPU::mepc(CPU::Reg(&setup_supervisor_environment));
//     CPU::mret();
// }

// __END_SYS

// void _setup() { Setup_Sifive_U::init(); }
