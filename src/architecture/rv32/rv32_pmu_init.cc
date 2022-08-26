// EPOS RISC-V 32 PMU Mediator Initialization

#include <architecture/rv32/rv32_pmu.h>

__BEGIN_SYS

void RV32_PMU::init()
{
    db<Init, PMU>(TRC) << "PMU::init()" << endl;

    mcounteren(CYCLES | TIME | INSTRUCTIONS_RETIRED);
}

__END_SYS
