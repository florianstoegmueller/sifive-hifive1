#include <stdio.h>
#include "cpu.h"

struct metal_cpu *cpu;
struct metal_interrupt *cpu_intr;
struct metal_interrupt *tmr_intr;

int cpu_init(){

    cpu = metal_cpu_get(metal_cpu_get_current_hartid());
    if (cpu == NULL) {
        printf("Abort. CPU is null.\n");
        return 0;
    }

    cpu_intr = metal_cpu_interrupt_controller(cpu);
    if (cpu_intr == NULL) {
        return 0;
    }
    metal_interrupt_init(cpu_intr);

    tmr_intr = metal_cpu_timer_interrupt_controller(cpu);
    if (tmr_intr == NULL) {
        return 0;
    }
    metal_interrupt_init(tmr_intr);

	return 1;
}

struct metal_cpu * get_cpu(){
    return cpu;
}

struct metal_interrupt * get_cpu_intr(){
    return cpu_intr;
}

struct metal_interrupt * get_tmr_intr(){
    return cpu_intr;
}