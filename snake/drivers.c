#include <stdio.h>
#include "drivers.h"

#define UART_DEVICE_NUM 0
#define GPIO_DEVICE_NUM 0
#define SPI_DEVICE_NUM 1

struct metal_cpu *cpu;
struct metal_uart *uart0;
struct metal_gpio *gpio0;
struct metal_spi *spi0;
struct metal_interrupt *cpu_intr;
struct metal_interrupt *tmr_intr;

int init_drivers(){
    cpu = metal_cpu_get(metal_cpu_get_current_hartid());
    if (cpu == NULL) {
        printf("Failed to get cpu\n");
        return 0;
    }

    cpu_intr = metal_cpu_interrupt_controller(cpu);
    if (cpu_intr == NULL) {
		printf("Failed to get software interrupt controller\n");
        return 0;
    }
    metal_interrupt_init(cpu_intr);

    tmr_intr = metal_cpu_timer_interrupt_controller(cpu);
    if (tmr_intr == NULL) {
		printf("Failed to get timer interrupt controller\n");
        return 0;
    }
    metal_interrupt_init(tmr_intr);

    uart0 = metal_uart_get_device(UART_DEVICE_NUM);
	if(uart0 == NULL) {
		printf("Failed to get uart device\n");
		return 0;
	}

	gpio0 = metal_gpio_get_device(GPIO_DEVICE_NUM);
	if(gpio0 == NULL) {
		printf("Failed to get gpio device\n");
		return 0;
	}

	spi0 = metal_spi_get_device(SPI_DEVICE_NUM);
	if(spi0 == NULL) {
		printf("Failed to get spi device\n");
		return 0;
	}

	return 1;
}

struct metal_cpu * get_cpu(){
    return cpu;
}

struct metal_uart * get_uart(){
    return uart0;
}

struct metal_gpio * get_gpio(){
    return gpio0;
}

struct metal_spi * get_spi(){
    return spi0;
}

struct metal_interrupt * get_cpu_intr(){
    return cpu_intr;
}

struct metal_interrupt * get_tmr_intr(){
    return tmr_intr;
}
