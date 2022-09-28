// EPOS SiFive-U (RISC-V) Memory Map

#ifndef __riscv_sifive_u_memory_map_h
#define __riscv_sifive_u_memory_map_h

#include <system/memory_map.h>

__BEGIN_SYS

struct Memory_Map
{
private:
    static const bool emulated = (Traits<CPU>::WORD_SIZE != 64); // specifying a SiFive-U with RV32 sets QEMU machine to Virt

public:
    enum : unsigned long {
        NOT_USED        = Traits<Machine>::NOT_USED,

        // Physical Memory
        RAM_BASE        = Traits<Machine>::RAM_BASE,
        RAM_TOP         = Traits<Machine>::RAM_TOP,
        MIO_BASE        = Traits<Machine>::MIO_BASE,
        MIO_TOP         = Traits<Machine>::MIO_TOP,
        INT_M2S         = RAM_TOP + 1 - 4096,   // the last page is used by the _int_m2s() interrupt forwarder installed by SETUP
        BOOT_STACK      = INT_M2S - Traits<Machine>::STACK_SIZE, // will be used as the stack's base, not the stack pointer
        FREE_BASE       = RAM_BASE,
        FREE_TOP        = BOOT_STACK,

        // Memory-mapped devices
        BIOS_BASE       = 0x00001000,   // BIOS ROM
        TEST_BASE       = 0x00100000,   // SiFive test engine
        RTC_BASE        = 0x00101000,   // Goldfish RTC
        UART0_BASE      = emulated ? 0x10000000 : 0x10010000, // NS16550A or SiFive UART
        CLINT_BASE      = 0x02000000,   // SiFive CLINT
        TIMER_BASE      = 0x02004000,   // CLINT Timer
        PLIIC_CPU_BASE  = 0x0c000000,   // SiFive PLIC
        PRCI_BASE       = emulated ? NOT_USED : 0x10000000,   // SiFive-U Power, Reset, Clock, Interrupt
        GPIO_BASE       = emulated ? NOT_USED : 0x10060000,   // SiFive-U GPIO
        OTP_BASE        = emulated ? NOT_USED : 0x10070000,   // SiFive-U OTP
        ETH_BASE        = emulated ? NOT_USED : 0x10090000,   // SiFive-U Ethernet
        FLASH_BASE      = 0x20000000,   // Virt / SiFive-U Flash

        // Physical Memory at Boot
        BOOT            = Traits<Machine>::BOOT,
        IMAGE           = Traits<Machine>::IMAGE,
        SETUP           = Traits<Machine>::SETUP,

        // Logical Address Space
        APP_LOW         = Traits<Machine>::APP_LOW,
        APP_HIGH        = Traits<Machine>::APP_HIGH,
        APP_CODE        = Traits<Machine>::APP_CODE,
        APP_DATA        = Traits<Machine>::APP_DATA,

        INIT            = Traits<Machine>::INIT,

        PHY_MEM         = Traits<Machine>::PHY_MEM,

        IO              = Traits<Machine>::IO,

        SYS             = Traits<Machine>::SYS,
        SYS_CODE        = NOT_USED,
        SYS_INFO        = NOT_USED,
        SYS_PT          = NOT_USED,
        SYS_PD          = NOT_USED,
        SYS_DATA        = NOT_USED,
        SYS_STACK       = NOT_USED,
        SYS_HEAP        = NOT_USED,
        SYS_HIGH        = NOT_USED
    };
};

__END_SYS

#endif
