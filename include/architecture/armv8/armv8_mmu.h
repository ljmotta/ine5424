// EPOS ARMv8 MMU Mediator Declarations

#ifndef __armv8_mmu_h
#define __armv8_mmu_h

#define __mmu_common_only__
#include <architecture/mmu.h>
#undef __mmu_common_only__
#include <system/memory_map.h>

__BEGIN_SYS

class MMU: public No_MMU {};

__END_SYS

#endif
