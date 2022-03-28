// See LICENSE for license details.

// executing out of SPI Flash at 0x20400000.

#include "helpers.h"
#include "cpu.h"
#include <metal/uart.h>
#include <metal/gpio.h>
#include <metal/machine/platform.h>

#define PIN_0_OFFSET 16 // TODO maybe get constant from somewhere else
#define RTC_FREQ METAL_FIXED_CLOCK_5_CLOCK_5_CLOCK_FREQUENCY // ==="===

#define _REG32(p, i) (*(volatile uint32_t *) ((p) + (i)))
#define GPIO_REG(offset) _REG32(METAL_SIFIVE_GPIO0_10012000_BASE_ADDRESS, offset)

struct metal_uart *uart0;
struct metal_gpio *gpio0;

void uart_init()
{
	// TODO (maybe) init registers
    uart0 = metal_uart_get_device(0);
	gpio0 = metal_gpio_get_device(0);
}

void _putc(char c) {
	metal_uart_putc(uart0, c);
}

int _getc(char * c){
	metal_uart_getc(uart0, (int *)c);
}


void _puts(const char * s) {
  while (*s != '\0'){
    _putc(*s++);
  }
}

void bitprint(uint32_t val)
{
	for(uint8_t i = 0; i < 32; i++)
	{
		_putc(val & (1 << (32 - i)) ? '1' : '0');
	}
	_puts("\r\n");
}


uint32_t mapPinToReg(uint8_t pin)
{
	if(pin < 8)
    {
        return pin + PIN_0_OFFSET;
    }
    if(pin >= 8 && pin < 14)
    {
        return pin - 8;
    }
    //ignoring non-wired pin 14 <==> 8
    if(pin > 14 && pin < 20)
    {
        return pin - 6;
    }
    return 0;
}

void setPinOutput(uint8_t pin)
{
	metal_gpio_disable_input(gpio0, mapPinToReg(pin));
	metal_gpio_enable_output(gpio0, mapPinToReg(pin));
	metal_gpio_clear_pin(gpio0, mapPinToReg(pin));
}

void setPinInput(uint8_t pin)
{
	setPinInputPullup(pin, 0);
}

void setPinInputPullup(uint8_t pin, uint8_t pullup_enable)
{
	metal_gpio_disable_pinmux(gpio0, mapPinToReg(pin));
	metal_gpio_disable_output(gpio0, mapPinToReg(pin));
	if(pullup_enable)
		GPIO_REG(METAL_SIFIVE_GPIO0_PUE)  |= (1 << mapPinToReg(pin));
	else
		GPIO_REG(METAL_SIFIVE_GPIO0_PUE)  &= ~(1 << mapPinToReg(pin));
	metal_gpio_enable_input(gpio0, mapPinToReg(pin));
}

/**
 * @param mode 0: rise, 1: fall, 2: high, 3: low
 */
void enableInterrupt(uint8_t pin, uint8_t mode)
{
	switch(mode)
	{
	case 0:
		metal_gpio_config_interrupt(gpio0, mapPinToReg(pin), METAL_GPIO_INT_RISING);
		break;
	case 1:
		metal_gpio_config_interrupt(gpio0, mapPinToReg(pin), METAL_GPIO_INT_FALLING);
		break;
	case 2:
		metal_gpio_config_interrupt(gpio0, mapPinToReg(pin), METAL_GPIO_INT_HIGH);
		break;
	case 3:
		metal_gpio_config_interrupt(gpio0, mapPinToReg(pin), METAL_GPIO_INT_LOW);
	default:
		_puts("Invalid interrupt mode");
	}
}

void setPin(uint8_t pin, uint8_t val)
{
	if(val)
	{
		metal_gpio_set_pin(gpio0, mapPinToReg(pin), val);
	}
	else
	{
		metal_gpio_clear_pin(gpio0, mapPinToReg(pin));
	}
}

uint8_t getPin(uint8_t pin)
{
	return metal_gpio_get_input_pin(gpio0, mapPinToReg(pin));
}

void printGPIOs()
{
	_puts("Output ENABLE ");
	bitprint(GPIO_REG(METAL_SIFIVE_GPIO0_OUTPUT_EN));
	_puts("Output  VALUE ");
	bitprint(GPIO_REG(METAL_SIFIVE_GPIO0_PORT));
	_puts("Input  ENABLE ");
	bitprint(GPIO_REG(METAL_SIFIVE_GPIO0_INPUT_EN));
	_puts("Input   VALUE ");
	bitprint(GPIO_REG(METAL_SIFIVE_GPIO0_VALUE));
}

void sleep_u(uint64_t micros)
{
	struct metal_cpu *cpu = get_cpu();
	volatile uint64_t then = metal_cpu_get_mtime(cpu) + ((micros * RTC_FREQ) / (1000 * 1000));
    while (metal_cpu_get_mtime(cpu) < then){}
}

void sleep(uint64_t millis)
{
    sleep_u(millis * 1000);
}

void setTimer(uint32_t millis)
{
	struct metal_cpu *cpu = get_cpu();
	unsigned long long then = metal_cpu_get_mtime(cpu) + ((millis * RTC_FREQ) / 1000);
	metal_cpu_set_mtimecmp(cpu, then);

    int tmr_id = metal_cpu_timer_get_interrupt_id(cpu);
	
    struct metal_interrupt *tmr_intr2;
	tmr_intr2 = metal_cpu_timer_interrupt_controller(cpu);
	metal_interrupt_enable(tmr_intr2, tmr_id);
}
