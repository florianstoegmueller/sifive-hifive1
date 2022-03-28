#ifndef CPU_H
#define CPU_H

#include <metal/cpu.h>

int cpu_init();

struct metal_cpu * get_cpu();
struct metal_interrupt * get_cpu_intr();
struct metal_interrupt * get_tmr_intr();

#endif