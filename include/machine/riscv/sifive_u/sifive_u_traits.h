// EPOS SiFive-U (RISC-V) Metainfo and Configuration

#ifndef __riscv_sifive_u_traits_h
#define __riscv_sifive_u_traits_h

#include <system/config.h>

__BEGIN_SYS

class Machine_Common;
template<> struct Traits<Machine_Common>: public Traits<Build>
{
protected:
    static const bool library = (Traits<Build>::MODE == Traits<Build>::LIBRARY);
};

template<> struct Traits<Machine>: public Traits<Machine_Common>
{
public:
    static const unsigned long NOT_USED          = 0xffffffffffffffff;  // Change to 64bits
    // Default Sizes and Quantities
    static const unsigned long MAX_THREADS       = 16;
    static const unsigned long STACK_SIZE        = 65536;   // 64 kB (64 * 1024)
    static const unsigned long HEAP_SIZE         = 0x100000;    // 1 MB
    static const unsigned long PAGE_SIZE_LEAF    = 0x200000;    // 2^21 2 MB
    static const unsigned long PAGE_SIZE         = 0x1000;      // 2^12 4 kB
    static const unsigned long PAGE_ENTRIES      = 512;         // 2^9

    // Physical Memory
    static const unsigned long RAM_BASE          = 0x0000000080000000;                           // 2 GB
    static const unsigned long RAM_TOP           = 0x00000000ffffffff;                           // 2 GB (0xFFFFFFFF - 0x80000000)
    static const unsigned long MIO_BASE          = 0x0000000000000000;
    static const unsigned long MIO_TOP           = 0x000000001fffffff;                            // 512 MB (max 512 MB of MIO => RAM + MIO < 2 GB)
    static const unsigned long BOOT_STACK        = RAM_TOP + 1 - STACK_SIZE;              // 64kB will be used as the stack's base, not the stack pointer
    static const unsigned long PAGE_TABLES       = (BOOT_STACK) - ((1 + PAGE_ENTRIES + (PAGE_ENTRIES * PAGE_ENTRIES)) * PAGE_SIZE); // reserve PAGE_TABLES on the begining of the ram (1 PNN[2] + 512 PNN[1])
    static const unsigned long FREE_BASE         = RAM_BASE;                              // Free memory from RAM_BASE + PAGE_TABLE
    static const unsigned long FREE_TOP          = BOOT_STACK;

    // Physical Memory at Boot
    // o setup é usado UMA vez, quando chega em supervisor, ele não é mais util
    // image - mkbi, para mais de um processo, tudo que tem depois da primeira aplicação
    // INIT = can't be on RAM_BASE?
    static const unsigned long BOOT              = NOT_USED;
    static const unsigned long SETUP             = NOT_USED;            // RAM_BASE (will be part of the free memory at INIT, using a logical address identical to physical eliminate SETUP relocation)
    static const unsigned long INIT              = NOT_USED;            // previous= RAM_BASE + 512 KB (will be part of the free memory at INIT)
    static const unsigned long IMAGE             = NOT_USED; // RAM_BASE + 1 MB (will be part of the free memory at INIT, defines the maximum image size; if larger than 3 MB then adjust at SETUP)
    static const unsigned long IO                = 0x0000000000000000;  // IO not being used

    // Logical Memory
    // Sv39, all bits from 63-39 must be equal to the bit 38
    // The first 0x400000 is reserved for the init;
    // why can't start at 0x0000000080400000 ??
    static const unsigned long APP_LOW           = 0x0000000080000000;      // ram is mapped to app 1:1
    static const unsigned long APP_CODE          = APP_LOW;                 // 
    static const unsigned long APP_DATA          = APP_CODE + 0x400000;     // data code should be below boot_stack
    static const unsigned long APP_HIGH          = 0x0000002fffffffff;      // 256 GB
    static const unsigned long PHY_MEM           = 0x0000003000000000;      // starts at RAM_BASE 1 <-> 1 0x3000000000 to 0x3fffffffff

    // Logical System Memory
    // Sv39, all bits from 63-39 must be equal to the bit 38
    // sys é passivo e esta em todos os address spaces
    // endereços lógicos mapeados 1:1 
    static const unsigned long SYS               = 0xffffffc000000000;      // 256 GB
    static const unsigned long SYS_CODE          = NOT_USED;
    static const unsigned long SYS_INFO          = NOT_USED;
    static const unsigned long SYS_PT            = NOT_USED;
    static const unsigned long SYS_PD            = NOT_USED;
    static const unsigned long SYS_DATA          = NOT_USED;
    static const unsigned long SYS_STACK         = NOT_USED;
    static const unsigned long SYS_HEAP          = NOT_USED;
    static const unsigned long SYS_HIGH          = NOT_USED;
};

template <> struct Traits<IC>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;
};

template <> struct Traits<Timer>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;

    static const unsigned int UNITS = 1;
    static const unsigned int CLOCK = 10000000;

    // Meaningful values for the timer frequency range from 100 to 10000 Hz. The
    // choice must respect the scheduler time-slice, i. e., it must be higher
    // than the scheduler invocation frequency.
    static const int FREQUENCY = 1000; // Hz
};

template <> struct Traits<UART>: public Traits<Machine_Common>
{
    static const unsigned int UNITS = 2;

    static const unsigned int CLOCK = 22729000;

    static const unsigned int DEF_UNIT = 1;
    static const unsigned int DEF_BAUD_RATE = 115200;
    static const unsigned int DEF_DATA_BITS = 8;
    static const unsigned int DEF_PARITY = 0; // none
    static const unsigned int DEF_STOP_BITS = 1;
};

template<> struct Traits<Serial_Display>: public Traits<Machine_Common>
{
    static const bool enabled = (Traits<Build>::EXPECTED_SIMULATION_TIME != 0);
    static const int ENGINE = UART;
    static const int UNIT = 1;
    static const int COLUMNS = 80;
    static const int LINES = 24;
    static const int TAB_SIZE = 8;
};

template<> struct Traits<Scratchpad>: public Traits<Machine_Common>
{
    static const bool enabled = false;
};

__END_SYS

#endif
