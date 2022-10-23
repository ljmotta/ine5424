// EPOS RV64 MMU Mediator Implementation

#include <architecture/rv64/rv64_mmu.h>

__BEGIN_SYS

MMU::List MMU::_free;
MMU::Page_Directory *MMU::_master;

__END_SYS
