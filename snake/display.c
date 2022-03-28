#include "display.h"

#include <stdio.h>
#include <stdlib.h>

#include "oled_shield.h"
#include "helpers.h"

#define MAX_SPI_FREQ   ( 8000000)

void spi_init(void)
{
	// TODO
}

void spi(uint8_t data)
{
	// TODO
}

void spi_complete()
{
	// TODO
}

void mode_data(void)
{
	// TODO
}

void mode_cmd(void)
{
	// TODO
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
	// TODO
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

