/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <metal/init.h>
#include <metal/csr.h>
#include "framebuffer.h"
#include "helpers.h"
#include "cpu.h"

int main() {
    uintptr_t mip_meip, mstatus_mie; // TODO maybe use encoding.h from riscv-opcodes
    metal_init();
    if(!cpu_init())
        return 1;

    mip_meip = (1 << 11);
    mstatus_mie = 0x00000008;

    //METAL_CPU_SET_CSR(mie, mip_meip);
	//METAL_CPU_SET_CSR(mstatus, mstatus_mie);

    printf("Hello, Snake!\n");
    sleep(3000);
    printf("Hello, Snake2!\n");

    return 0;
}
