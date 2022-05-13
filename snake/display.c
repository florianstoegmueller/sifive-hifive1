#include <stdio.h>
#include <stdlib.h>
#include <metal/machine.h>
#include <metal/io.h>
#include <metal/spi.h>
#include "display.h"
#include "helpers.h"
#include "drivers.h"
#include "oled_shield.h"

#define MAX_SPI_FREQ   ( 8000000)
#define METAL_SPI_TXWM 1

#define SPI_FMT_PROTO(x)        ((x) & 0x3)
#define SPI_FMT_ENDIAN(x)       (((x) & 0x1) << 2)
#define SPI_FMT_DIR(x)          (((x) & 0x1) << 3)
#define SPI_FMT_LEN(x)          (((x) & 0xf) << 16)

#define SPI_DIR_RX              0
#define SPI_DIR_TX              1

#define SPI_PROTO_S             0
#define SPI_PROTO_D             1
#define SPI_PROTO_Q             2

#define SPI_ENDIAN_MSB          0
#define SPI_ENDIAN_LSB          1

struct metal_spi_config config;
unsigned long base;

void spi_init(void)
{
	base = __metal_driver_sifive_spi0_control_base(get_spi());

	//Pins enable output
	metal_gpio_disable_input(get_gpio(), mapPinToReg(OLED_SDIN));
	metal_gpio_disable_input(get_gpio(), mapPinToReg(OLED_SCLK));
	metal_gpio_disable_input(get_gpio(), mapPinToReg(OLED_CS));

	metal_gpio_enable_output(get_gpio(), mapPinToReg(OLED_SDIN));
	metal_gpio_enable_output(get_gpio(), mapPinToReg(OLED_SCLK));
	metal_gpio_enable_output(get_gpio(), mapPinToReg(OLED_CS));

	// Select IOF SPI1.MOSI [SDIN] and SPI1.SCK [SLCK] and SPI1.SS0 [CS]
	__METAL_ACCESS_ONCE((__metal_io_u32 *) (base + METAL_SIFIVE_GPIO0_IOF_SEL)) &= ~((1 << mapPinToReg(OLED_SDIN)) |
																					(1 << mapPinToReg(OLED_SCLK)) |
																					(1 << mapPinToReg(OLED_CS)));

	__METAL_ACCESS_ONCE((__metal_io_u32 *) (base + METAL_SIFIVE_GPIO0_IOF_EN)) |= ((1 << mapPinToReg(OLED_SDIN)) |
																					(1 << mapPinToReg(OLED_SCLK)) |
																					(1 << mapPinToReg(OLED_CS)));							

	metal_gpio_set_pin(get_gpio(), mapPinToReg(OLED_SDIN), 1);
	metal_gpio_set_pin(get_gpio(), mapPinToReg(OLED_SCLK), 1);
	metal_gpio_set_pin(get_gpio(), mapPinToReg(OLED_CS), 1);

	// Set up SPI controller
	long cpu_clock = metal_clock_get_rate_hz(&__metal_dt_clock_4.clock);
	__METAL_ACCESS_ONCE((__metal_io_u32 *) (base + METAL_SIFIVE_SPI0_SCKDIV)) = ((cpu_clock / MAX_SPI_FREQ) - 1);
	__METAL_ACCESS_ONCE((__metal_io_u32 *) (base + METAL_SIFIVE_SPI0_SCKMODE)) = 0;
	__METAL_ACCESS_ONCE((__metal_io_u32 *) (base + METAL_SIFIVE_SPI0_CSID)) = OLED_CS_OFS;
	__METAL_ACCESS_ONCE((__metal_io_u32 *) (base + METAL_SIFIVE_SPI0_CSDEF)) = 0xffff;
	__METAL_ACCESS_ONCE((__metal_io_u32 *) (base + METAL_SIFIVE_SPI0_CSMODE)) = 2;
	__METAL_ACCESS_ONCE((__metal_io_u32 *) (base + METAL_SIFIVE_SPI0_FMT)) = SPI_FMT_PROTO(SPI_PROTO_S) | SPI_FMT_ENDIAN(SPI_ENDIAN_MSB) | SPI_FMT_DIR(SPI_DIR_TX) | SPI_FMT_LEN(8);
	__METAL_ACCESS_ONCE((__metal_io_u32 *) (base + METAL_SIFIVE_SPI0_TXMARK)) = 1;
	__METAL_ACCESS_ONCE((__metal_io_u32 *) (base + METAL_SIFIVE_SPI0_IE)) = METAL_SPI_TXWM;
}

void spi(uint8_t data)
{
	while (__METAL_ACCESS_ONCE((__metal_io_u32 *) (base + METAL_SIFIVE_SPI0_TXDATA)) & (1 << 31))
	{
		asm volatile("nop");
	}
	__METAL_ACCESS_ONCE((__metal_io_u8 *) (base + METAL_SIFIVE_SPI0_TXDATA)) = data;
}

void spi_complete()
{
	while (!(__METAL_ACCESS_ONCE((__metal_io_u32 *) (base + METAL_SIFIVE_SPI0_IP))
			& METAL_SPI_TXWM))
        asm volatile("nop");
	sleep_u(10);
}

void mode_data(void)
{
	//not already in data mode
    long gpio_output_val = get_gpio()->vtable->output(get_gpio());
	if(!((gpio_output_val) & (1 << mapPinToReg(OLED_DC))))
	{
		spi_complete(); /* wait for SPI to complete before toggling */
		setPin(OLED_DC, 1);
	}
}

void mode_cmd(void)
{
	//not already in command mode
    long gpio_output_val = get_gpio()->vtable->output(get_gpio());
	if(gpio_output_val & (1 << mapPinToReg(OLED_DC)))
	{
		spi_complete(); /* wait for SPI to complete before toggling */
		setPin(OLED_DC, 0);
	}
}

void setDisplayOn(uint8_t on)
{
	mode_cmd();
	spi(0xAE | (on & 1));
}

void setChargePumpVoltage(uint8_t voltage)
{
	mode_cmd();
	spi(0b00110000 | (voltage & 0b11));
}

void invertColor(uint8_t invert)
{
	mode_cmd();
	spi(0b10100110 | (invert & 1));	//set 'normal' direction (1 = bright)
}

void setEntireDisplayOn(uint8_t allWhite)
{
	mode_cmd();
	spi(0b10100100 | (allWhite & 1));
}

void setDisplayStartLine(uint8_t startline)
{
	mode_cmd();
	spi(0b01000000 | (startline & 0b111111));
}

void setDisplayOffset(uint8_t something)
{
	mode_cmd();
    spi(0b11010011);	//double byte to set display offset to (y)0;
    spi(0b00000000);	//double byte to set display offset to (y)0;
}

void flipDisplay(uint8_t flip)
{
	mode_cmd();
	spi(0b11000000 | (0b1111 * (flip & 1)));
}

void setContrast(uint8_t contrast)
{
	/**
	 * Segment output current setting: ISEG = a/256* IREF * scale_factor
	 * Where a is contrast step, IREF is reference current (12.5uA), scale_factor = 16
	 */
	mode_cmd();
	spi(0b10000001);	//Contrast mode
	spi(contrast);
}

void fadeIn(uint64_t millis)
{
	for(uint8_t contrast = 0; contrast != 0xff; contrast++)
	{
		setContrast(contrast);
		//printf("Fade in: Sleeping for %lu micros\r\n", (millis * 1000) / 256);
		sleep_u((millis * 1000) / 256);
	}
}

void fadeOut(uint64_t millis)
{
	for(uint8_t contrast = 0xff; contrast != 0; contrast--)
	{
		setContrast(contrast);
		//printf("Fade out: Sleeping for %lu micros\r\n", (millis * 1000) / 256);
		sleep_u((millis * 1000) / 256);
	}
}

/** Initialize pmodoled module */
void oled_init()
{
	spi_init();

    // Initial setup
    //
    //Enable RESET and D/C Pin
	metal_gpio_disable_input(get_gpio(), mapPinToReg(OLED_RES));
	metal_gpio_disable_input(get_gpio(), mapPinToReg(OLED_DC));
	metal_gpio_enable_output(get_gpio(), mapPinToReg(OLED_RES));
	metal_gpio_enable_output(get_gpio(), mapPinToReg(OLED_DC));

	setPin(OLED_DC, 0);

	//RESET
	setPin(OLED_RES,0);
	sleep_u(10);		//min 10us
	setPin(OLED_RES,1);
	sleep(100);		//'about 100ms'

    // Command mode
	mode_cmd();

	// Initialize display to desired operating mode.

    setChargePumpVoltage(0b10);
    invertColor(0);
    //flipDisplay(1);
    setContrast(0xff);		//MY EYES!!!

    // 4. Clear screen (entire memory)
    oled_clear();

    setDisplayOn(1);
}

void set_x(unsigned col)
{
    mode_cmd();
    spi(0x00 | ((col+DISP_W_OFFS) & 0xf));
    spi(0x10 | (((col+DISP_W_OFFS) >> 4)&0xf));
    mode_data();
}
void set_row(unsigned row)
{
    mode_cmd();
    spi(0xb0 | (row & 0x7));
    mode_data();
}

void set_xrow(unsigned col, unsigned row)
{
    mode_cmd();
    spi(0x00 | ((col+DISP_W_OFFS) & 0xf));
    spi(0x10 | (((col+DISP_W_OFFS) >> 4)&0xf));
    spi(0xb0 | (row & 0x7));
    mode_data();
}

void oled_clear(void)
{
	for (unsigned y = 0; y < DISP_H/8; ++y) {
    	set_xrow(0, y);
		for (unsigned x=0; x < DISP_W; ++x) {
    		spi(0x00);
    	}
    }
	set_xrow(0,0);
}

