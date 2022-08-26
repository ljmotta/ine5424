// EPOS RISC-V 64 CPU Mediator Implementation

#include <architecture/rv64/rv64_cpu.h>
#include <system.h>

__BEGIN_SYS

unsigned int CPU::_cpu_clock;
unsigned int CPU::_bus_clock;

void CPU::Context::save() volatile
{
    ASM("       sd       x3,    0(a0)           \n"     // push garbage for USP
        "       sd       x1,    8(a0)           \n"     // push RA as PC
        "       csrr     x3,  mstatus           \n"
        "       sd       x3,   16(sp)           \n"     // push ST
        "       sd       x1,   24(sp)           \n"     // push RA
        "       sd       x5,   32(sp)           \n"     // push x5-x31
        "       sd       x6,   40(sp)           \n"
        "       sd       x7,   48(sp)           \n"
        "       sd       x8,   56(sp)           \n"
        "       sd       x9,   64(sp)           \n"
        "       sd      x10,   72(sp)           \n"
        "       sd      x11,   80(sp)           \n"
        "       sd      x12,   88(sp)           \n"
        "       sd      x13,   96(sp)           \n"
        "       sd      x14,  104(sp)           \n"
        "       sd      x15,  112(sp)           \n"
        "       sd      x16,  120(sp)           \n"
        "       sd      x17,  128(sp)           \n"
        "       sd      x18,  136(sp)           \n"
        "       sd      x19,  144(sp)           \n"
        "       sd      x20,  152(sp)           \n"
        "       sd      x21,  160(sp)           \n"
        "       sd      x22,  168(sp)           \n"
        "       sd      x23,  176(sp)           \n"
        "       sd      x24,  184(sp)           \n"
        "       sd      x25,  192(sp)           \n"
        "       sd      x26,  200(sp)           \n"
        "       sd      x27,  208(sp)           \n"
        "       sd      x28,  216(sp)           \n"
        "       sd      x29,  224(sp)           \n"
        "       sd      x30,  232(sp)           \n"
        "       sd      x31,  240(sp)           \n"
        "       ret                             \n");
}

// Context load does not verify if interrupts were previously enabled by the Context's constructor
// We are setting mstatus to MPP | MPIE, therefore, interrupts will be enabled only after mret
void CPU::Context::load() const volatile
{
    sp(Log_Addr(this));
    pop();
    iret();
}

void CPU::switch_context(Context ** o, Context * n)     // "o" is in a0 and "n" is in a1
{   
    // Push the context into the stack and update "o"
    Context::push();
    ASM("sd sp, 0(a0)");   // update Context * volatile * o, which is in a0

    // Set the stack pointer to "n" and pop the context from the stack
    ASM("mv sp, a1");   // "n" is in a1
    Context::pop();
    iret();
}

__END_SYS

