// EPOS IA32 MMU Mediator Implementation

#include <architecture/rv64/rv64_mmu.h>

__BEGIN_SYS

Sv39_MMU::List Sv39_MMU::_free;
Sv39_MMU::Page_Directory *Sv39_MMU::_master;

__END_SYS
