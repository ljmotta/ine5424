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
        BOOT_STACK      = Traits<Machine>::BOOT_STACK, // will be used as the stack's base, not the stack pointer
        FREE_BASE       = Traits<Machine>::FREE_BASE,
        FREE_TOP        = Traits<Machine>::FREE_TOP,

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
        SYS_CODE        = Traits<Machine>::SYS_CODE,
        SYS_INFO        = Traits<Machine>::SYS_INFO,
        SYS_PT          = Traits<Machine>::SYS_PT,
        SYS_PD          = Traits<Machine>::SYS_PD,
        SYS_DATA        = Traits<Machine>::SYS_DATA,
        SYS_STACK       = Traits<Machine>::SYS_STACK,
        SYS_HEAP        = Traits<Machine>::SYS_HEAP,
        SYS_HIGH        = Traits<Machine>::SYS_HIGH
    };
};

__END_SYS

#endif
