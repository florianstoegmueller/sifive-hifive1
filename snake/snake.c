/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <metal/init.h>
#include <metal/csr.h>
#include "framebuffer.h"
#include "drivers.h"
#include "helpers.h"

#include "display.h"

int main() {
    uintptr_t mip_meip, mstatus_mie; // TODO maybe use encoding.h from riscv-opcodes
    metal_init();
    if(!init_drivers())
        return 1;

    mip_meip = (1 << 11);
    mstatus_mie = 0x00000008;

    //METAL_CPU_SET_CSR(mie, mip_meip);
	//METAL_CPU_SET_CSR(mstatus, mstatus_mie);

    printf("Hello, Snake!\n");
    //sleep(3000);
    printf("Hello, Snake2!\n");

    long value = get_gpio()->vtable->output(get_gpio());
    printf("rate: %x\n", value);
    metal_gpio_set_pin(get_gpio(), 1, 1);
    value = get_gpio()->vtable->output(get_gpio());
    printf("rate: %x\n", value);
    metal_gpio_set_pin(get_gpio(), 4, 1);
    value = get_gpio()->vtable->output(get_gpio());
    printf("rate: %x\n", value);


    /*struct metal_spi  *spi = metal_spi_get_device(0);
    int r = metal_spi_get_baud_rate(spi);
    printf("rate: %d\n", r);
    int s = metal_spi_set_baud_rate(spi, 100);
    printf("s: %d\n", s);
    r = metal_spi_get_baud_rate(spi);
    printf("rate: %d\n", r);*/

    printGPIOs();

    spi(0x55);

    uint8_t data = 5;
	char tx_buf[1] = {data};
	char* c = (char*) &data;
    printf("%d\n", data);
    printf("%d\n", tx_buf);
    printf("%d\n", c);
    printf("%d\n", tx_buf[0]);
    printf("%d\n", c[0]);

    printf("Size of is %lu\n", sizeof(data));
    printf("Size of is %lu\n", sizeof(tx_buf));
    printf("Size of is %lu\n", sizeof(c));
    printf("Size of is %lu\n", sizeof(tx_buf[0]));
    printf("Size of is %lu\n", sizeof(c[0]));

    return 0;
}
