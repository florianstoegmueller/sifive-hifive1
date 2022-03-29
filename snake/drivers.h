#ifndef DRIVERS_H
#define DRIVERS_H

#include <metal/cpu.h>
#include <metal/uart.h>
#include <metal/gpio.h>
#include <metal/spi.h>

#define CPU_FREQ 65000000

int init_drivers();

struct metal_cpu * get_cpu();
struct metal_uart * get_uart();
struct metal_gpio * get_gpio();
struct metal_spi * get_spi();
struct metal_interrupt * get_cpu_intr();
struct metal_interrupt * get_tmr_intr();

#endif