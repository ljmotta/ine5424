// EPOS RISC-V Mediator Implementation

#include <machine/machine.h>
#include <machine/display.h>

__BEGIN_SYS

void Machine::panic()
{
    CPU::int_disable();

    if(Traits<Display>::enabled)
        Display::puts("PANIC!\n");

    if(Traits<System>::reboot)
        reboot();
    else
        poweroff();
}

void Machine::reboot()
{
    if(Traits<System>::reboot) {
        db<Machine>(WRN) << "Machine::reboot()" << endl;

#ifdef __sifive_e__
        CPU::Reg * reset = reinterpret_cast<CPU::Reg *>(Memory_Map::AON_BASE);
        reset[0] = 0x5555;
#endif

#if defined(__sifive_u__) && defined(__rv32__)
        CPU::Reg * reset = reinterpret_cast<CPU::Reg *>(Memory_Map::TEST_BASE);
        reset[0] = 0x5555;
#endif

        while(true);
    } else {
        poweroff();
    }
}

void Machine::poweroff()
{
    db<Machine>(WRN) << "Machine::poweroff()" << endl;

#ifdef __sifive_e__
        CPU::Reg * reset = reinterpret_cast<CPU::Reg *>(Memory_Map::AON_BASE);
        reset[0] = 0x5555;
#endif

#if defined(__sifive_u__) && defined(__rv32__)
        CPU::Reg * reset = reinterpret_cast<CPU::Reg *>(Memory_Map::TEST_BASE);
        reset[0] = 0x5555;
#endif

    while(true);
}

__END_SYS
