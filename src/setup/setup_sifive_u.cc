// EPOS SiFive-U (RISC-V) SETUP

#include <architecture.h>
#include <machine.h>
#include <utility/elf.h>
#include <utility/string.h>


using namespace EPOS::S;
typedef unsigned long Reg;

// timer handler
extern "C" [[gnu::interrupt, gnu::aligned(8)]] void _mmode_forward() {
    Reg id = CPU::mcause();
    if((id & CLINT::INT_MASK) == CLINT::IRQ_MAC_TIMER) {
        Timer::reset();
        CPU::sie(CPU::STI);
    }
    Reg interrupt_id = 1 << ((id & CLINT::INT_MASK) - 2);
    if(CPU::int_enabled() && (CPU::sie() & (interrupt_id)))
        CPU::mip(interrupt_id);
}

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
    static const unsigned long PHY_MEM          = Memory_Map::PHY_MEM;
    static const unsigned long RAM_BASE         = Memory_Map::RAM_BASE;
    static const unsigned long RAM_TOP          = Memory_Map::RAM_TOP;
    static const unsigned long MIO_BASE         = Memory_Map::MIO_BASE;
    static const unsigned long MIO_TOP          = Memory_Map::MIO_TOP;
    static const unsigned long FREE_BASE        = Memory_Map::FREE_BASE;
    static const unsigned long FREE_TOP         = Memory_Map::FREE_TOP;
    static const unsigned long SETUP            = Memory_Map::SETUP;
    static const unsigned long BOOT_STACK       = Memory_Map::BOOT_STACK;
    static const unsigned long PAGE_TABLES      = Memory_Map::PAGE_TABLES;
    static const unsigned long IO               = Memory_Map::IO;
    static const unsigned long SYS              = Memory_Map::SYS;
    static const unsigned long SYS_INFO         = Memory_Map::SYS_INFO;
    static const unsigned long SYS_PT           = Memory_Map::SYS_PT;
    static const unsigned long SYS_PD           = Memory_Map::SYS_PD;
    static const unsigned long SYS_CODE         = Memory_Map::SYS_CODE;
    static const unsigned long SYS_DATA         = Memory_Map::SYS_DATA;
    static const unsigned long SYS_STACK        = Memory_Map::SYS_STACK;
    static const unsigned long APP_CODE         = Memory_Map::APP_CODE;
    static const unsigned long APP_DATA         = Memory_Map::APP_DATA;
    static const unsigned long PAGE_ENTRIES      = Traits<Machine>::PAGE_ENTRIES;               // quantidade de entradas numa página
    static const unsigned long PAGE_SIZE         = Traits<Machine>::PAGE_SIZE;  // 8 * 512 = 4kb
    static const unsigned long PT_ENTRIES_LV0    = 512;               // 512 entradas de 4kB
    static const unsigned long PD_ENTRIES_LV1    = 512;                // 32 entradas de 2MB
    static const unsigned long PD_ENTRIES_LV2    = 512;                 // 1 entrada LV2
    static const unsigned long ENTRIES_SIZE      = (1 + PD_ENTRIES_LV2 + PD_ENTRIES_LV1 + PT_ENTRIES_LV0) * PAGE_SIZE; // 0x221000 ~2MB

    // Architecture Imports
    typedef CPU::Reg Reg;
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::Log_Addr Log_Addr;
    typedef MMU::Flags RV64_Flags;
    typedef MMU::Page_Table Page_Table;
    typedef MMU::Page_Directory Page_Directory;
    typedef MMU::PT_Entry PT_Entry;
    typedef MMU::Page Page;

    // System_Info Imports
    typedef System_Info::Boot_Map BM;
    typedef System_Info::Physical_Memory_Map PMM;


public:
    Setup();

private:
    void say_hi();
    void enable_paging();
    void call_next();
    void panic() { Machine::panic(); }

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

    enable_paging();

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

void Setup::enable_paging()
{
    db<Setup>(TRC) << "Setup::enable_paging()" << endl;

    // 1024
    unsigned long page_tables = MMU::page_tables(MMU::pages(Memory_Map::RAM_TOP - Memory_Map::RAM_BASE));

    // local das tabelas lv2 e lv1
    unsigned long *page_tables_location = reinterpret_cast<unsigned long*>((PAGE_TABLES + sizeof(MMU::Page) - 1) & ~(sizeof(MMU::Page) - 1));
    
    // o ponteiro para a primeira tabela lv2
    Page_Directory* _pd_master = MMU::current();
    _pd_master = new (page_tables_location) Page_Directory();

    // avançamos 512 * 8 (long), aritmética de ponteiros
    page_tables_location = reinterpret_cast<unsigned long *>(reinterpret_cast<unsigned long>(page_tables_location) + (PAGE_ENTRIES * 8));
    // cria ptes, com flag valid nos endereços, shiftando de 512?
    _pd_master->remap_d(page_tables_location, 0, PAGE_ENTRIES);
    // pte's addresses
    // addr = page_tables_location
    // addr = page_tables_location + 0x1000
    // addr = page_tables_location + 0x2000
    // ....
    // addr = page_tables_location + 0x200000

    page_tables_location = reinterpret_cast<unsigned long *>(reinterpret_cast<unsigned long>(page_tables_location) + PAGE_ENTRIES);
    unsigned long *mem = reinterpret_cast<unsigned long*>(RAM_BASE);

    // recuperamos o endereço do primeiro elemento da PD, e shiftamos de 2 para esquerda
    // realizando o alinhamento a 12 bits.
    for (unsigned long i = 0; i < PD_ENTRIES_LV1; i++) { // 0..511
        Page_Directory * pd_lv1 = new (reinterpret_cast<unsigned long *>(reinterpret_cast<unsigned long>(page_tables_location) + (0x1000UL * (i + 1)))) Page_Directory();
        // pd_lv1_location/addr = page_tables_location + 0x200000
        // addr = page_tables_location + 0x201000
        // ....
        // pd_lv1_location/addr = page_tables_location + 0x400000
        // addr = page_tables_location + 0x401000
        // ...
        // pd_lv1_location = page_tables_location + 0x600000
        unsigned long * pd_lv1_location = reinterpret_cast<unsigned long *>(reinterpret_cast<unsigned long>(page_tables_location) + (0x200000UL * (i + 1)));
        // db<Setup>(INF) << "i=" << i << ", pd_lv1_location=" << pd_lv1_location << endl;
        pd_lv1->remap_d(pd_lv1_location, 0, PAGE_ENTRIES);
        for (unsigned long j = 0; j < PT_ENTRIES_LV0; j++) { // 0..511
            unsigned long * pd_lv0_location = reinterpret_cast<unsigned long *>(reinterpret_cast<unsigned long>(pd_lv1_location) + ((0x1000UL) * (j + 1)));
            Page_Table * pt_lv0 = new (pd_lv0_location) Page_Table();
            // db<Setup>(INF) << "j=" << j << ", pd_lv0_location=" << pd_lv0_location << endl;
            
            // addr = RAM_BASE + 0x200000
            // addr = RAM_BASE + 0x201000
            // addr = RAM_BASE + 0x400000
            // addr = RAM_BASE + 0x400000
            // ....
            // addr = RAM_BASE + 0x400000
            pt_lv0->remap(mem, 0, PAGE_ENTRIES, RV64_Flags::SYS);
            mem = reinterpret_cast<unsigned long *>(reinterpret_cast<unsigned long>(mem) + PAGE_ENTRIES);
        }
    }

    // Set SATP and enable paging
    // CPU::satp((1UL << 63) | (reinterpret_cast<unsigned long>(_pd_master) >> 12));

    // Flush TLB to ensure we've got the right memory organization
    MMU::flush_tlb();
}

void Setup::call_next() {
    db<Setup>(INF) << "SETUP almost ready!" << endl;

    CPU::sie(CPU::SSI | CPU::STI | CPU::SEI);
    CPU::sstatus(CPU::SPP_S);

    CPU::sepc(CPU::Reg(&_start));
    CLINT::stvec(CLINT::DIRECT, CPU::Reg(&_int_entry));

    CPU::sret();
    db<Setup>(ERR) << "OS failed to init!" << endl;
}

__END_SYS

using namespace EPOS::S;

void _entry() // machine mode
{
    // SiFive-U core 0 doesn't have MMU
    if(CPU::mhartid() == 0)
        CPU::halt();

    // ensure that sapt is 0
    CPU::satp(0);
    Machine::clear_bss();

    // need to check?
    // set the stack pointer, thus creating a stack for SETUP
    CPU::sp(Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE - sizeof(long));

    // Set up the Physical Memory Protection registers correctly
    // A = NAPOT, X, R, W
    CPU::pmpcfg0(0x1f);
    // All memory
    CPU::pmpaddr0((1UL << 55) - 1);

    // Delegate all traps to supervisor
    // Timer will not be delegated due to architecture reasons.
    CPU::mideleg(CPU::SSI | CPU::STI | CPU::SEI);
    CPU::medeleg(0xffff);

    CPU::mies(CPU::MSI | CPU::MTI | CPU::MEI);              // enable interrupts generation by CLINT
    CPU::mint_disable();                                    // (mstatus) disable interrupts (they will be reenabled at Init_End)
    CLINT::mtvec(CLINT::DIRECT, CPU::Reg(&_mmode_forward)); // setup a preliminary machine mode interrupt handler pointing it to _mmode_forward

    // MPP_S = change to supervirsor
    // MPIE = otherwise we won't ever receive interrupts
    CPU::mstatus(CPU::MPP_S | CPU::MPIE);
    CPU::mepc(CPU::Reg(&_setup));                       // entry = _setup
    CPU::mret();                                        // enter supervisor mode at setup (mepc) with interrupts enabled (mstatus.mpie = true)
}

void _setup() // supervisor mode
{
    kerr << endl;
    kout << endl;

    Setup setup;
}
