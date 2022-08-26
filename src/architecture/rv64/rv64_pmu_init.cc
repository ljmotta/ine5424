// EPOS RISC-V 64 PMU Mediator Initialization

#include <architecture/rv64/rv64_pmu.h>

__BEGIN_SYS

void RV64_PMU::init()
{
    db<Init, PMU>(TRC) << "PMU::init()" << endl;

    mcounteren(CYCLES | TIME | INSTRUCTIONS_RETIRED);
}

__END_SYS
