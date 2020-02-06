// See LICENSE for license details.

// executing out of SPI Flash at 0x20400000.

#include "helpers.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "plic/plic_driver.h"
#include "encoding.h"	//For CSRs
#include "sifive/devices/spi.h"

#include "oled_shield.h"
#include "font.h"
#include "display.h"
#include "framebuffer.h"

#define MAX_SNAKE_LENGTH 256
#define MAX_DELAY 100

const uint8_t buttons[] =
{
	BUTTON_D,
	BUTTON_CTR,
	BUTTON_R,
	BUTTON_L,
	BUTTON_U,
	BUTTON_A,
	BUTTON_B
};

struct Food{
	uint8_t x, y;
	uint8_t valid;
};

struct State{
	enum Direction{
		up = 0,
		down,
		left,
		right
	} direction, nextDirection;
	uint8_t length;
	uint8_t intelligent;
	struct Food food;
} state;

struct Snakesegment
{
	uint8_t x, y;
};
struct Snakesegment snake[MAX_SNAKE_LENGTH];


// Global Instance data for the PLIC
// for use by the PLIC Driver.
plic_instance_t g_plic;
// Structures for registering different interrupt handlers
// for different parts of the application.
typedef void (*interrupt_function_ptr_t) (plic_source);

//array of function pointers which contains the PLIC
//interrupt handlers
interrupt_function_ptr_t g_ext_interrupt_handlers[PLIC_NUM_INTERRUPTS];


/*Entry Point for PLIC Interrupt Handler*/
void handle_m_ext_interrupt()
{
    plic_source int_num  = PLIC_claim_interrupt(&g_plic);
	if ((int_num >=1 ) && (int_num < PLIC_NUM_INTERRUPTS))
	{
		g_ext_interrupt_handlers[int_num](int_num);
	}
	else
	{
		//exit(1 + (uintptr_t) int_num);
		printf("unhandled Interrupt %u", int_num);
		while(1){};
	}
	PLIC_complete_interrupt(&g_plic, int_num);
	//puts("completed interrupt\r\n");
}

void button_handler(plic_source int_num)
{
	int_num -= INT_GPIO_BASE;
	if(int_num ==  mapPinToReg(BUTTON_D))
	{
		if(state.direction != up)
			state.nextDirection = down;
	}
	else if(int_num == mapPinToReg(BUTTON_U))
	{
		if(state.direction != down)
			state.nextDirection = up;
	}
	else if(int_num == mapPinToReg(BUTTON_L))
	{
		if(state.direction != right)
			state.nextDirection = left;
	}
	else if(int_num == mapPinToReg(BUTTON_R))
	{
		if(state.direction != left)
			state.nextDirection = right;
	}
	else if(int_num == mapPinToReg(BUTTON_A))
	{
		state.intelligent ^= 1;
	}
	else
	{
	}
	GPIO_REG(GPIO_RISE_IP) |= (1 << int_num);
	GPIO_REG(GPIO_FALL_IP) |= (1 << int_num);
}

//default empty PLIC handler
void invalid_global_isr(plic_source int_num)
{
	printf("Unexpected global interrupt %d!\r\n", int_num);
}

void setup_button_irq()
{
    PLIC_init(&g_plic,
            PLIC_CTRL_ADDR,
            PLIC_NUM_INTERRUPTS,
            PLIC_NUM_PRIORITIES);

	for(unsigned i = 0; i < sizeof(buttons); i++)
	{
		setPinInputPullup(buttons[i], 1);
		//enableInterrupt(buttons[i], 0);	//rise necessary?
		enableInterrupt(buttons[i], 1);	//fall
	    PLIC_enable_interrupt (&g_plic, INT_GPIO_BASE + mapPinToReg(buttons[i]));
	    PLIC_set_priority(&g_plic, INT_GPIO_BASE + mapPinToReg(buttons[i]), 2+i);
	    g_ext_interrupt_handlers[INT_GPIO_BASE + mapPinToReg(buttons[i])] = button_handler;
	    printf("Inited button %d\r\n", buttons[i]);
	}
}

void handle_m_time_interrupt()
{
	clear_csr(mie, MIP_MTIP);
}

uint8_t get_random_number()
{
	static uint8_t rand = 15;	//generated by fair dice roll
	rand += *(volatile uint64_t*)(CLINT_CTRL_ADDR + CLINT_MTIME);
	//rand = rand * rand;
	return rand;
}

void spawn_food()
{
	while(!state.food.valid)
	{
		state.food.x = get_random_number()%DISP_W;
		state.food.y = get_random_number()%DISP_H;
		
		state.food.valid = 1;
		for(unsigned i = 0; i < state.length; i++)
		{
			if(snake[i].x == state.food.x && snake[i].y == state.food.y)
				state.food.valid = 0;
		}
	}
	fb_set_pixel_direct(state.food.x, state.food.y, 1);
	printf("New food at %u %u\n", state.food.x, state.food.y);
}

void think()
{
	if(state.food.valid)
	{
		int diffx = state.food.x - snake[0].x;
		int diffy = state.food.y - snake[0].y;
		if(diffy > DISP_H/2)
			diffy *= -1;
		if(diffx > DISP_W/2)
			diffx *= -1;

		state.nextDirection = state.direction;
		if(abs(diffx) > 0)
		{
			if(diffx > 0)
				if(state.direction != left)
					state.nextDirection = right;
			if(diffx < 0)
				if(state.direction != right)
					state.nextDirection = left;
		}
		else
		{
			if(diffy > 0)
				if(state.direction != up)
					state.nextDirection = down;
			if(diffy < 0)
				if(state.direction != down)
					state.nextDirection = up;
		}
		for(unsigned tries = 0; tries < 5; tries++)
		{
			int goingToHit = 0;
			struct Snakesegment future = snake[0];
			future.x += state.nextDirection == left ? -1 : state.nextDirection == right ? 1 : 0;
			future.y += state.nextDirection == down ? 1 : state.nextDirection == up ? -1 : 0;

			for(uint8_t i = 1; i < state.length; i++)
			{
				if(snake[i].x == future.x && snake[i].y == future.y)
				{
					goingToHit = 1;
					break;
				}
			}
			if(goingToHit)
		        {
				//printf("Direction %d at try %d is not ok\n", state.nextDirection, tries);
				state.nextDirection++;
				state.nextDirection = state.nextDirection%4;
			}
			else
			{
				//printf("Direction %d at try %d is     ok\n", state.nextDirection, tries);
				break;
			}
		}
	}
}

void reset_state()
{
	state.direction = left;
	state.nextDirection = left;
	state.length = 10;
	state.intelligent = 1;
	spawn_food();
	memset(snake, 0, sizeof(struct Snakesegment) * MAX_SNAKE_LENGTH);
	for(unsigned i = 0; i < state.length; i++)
	{
		snake[i].x = (DISP_W/2)+i;
		snake[i].y = DISP_H/2;
		fb_set_pixel(snake[i].x, snake[i].y, 1);
	}
	fb_flush();
}

uint8_t snake_step()
{
	struct Snakesegment buf = snake[0];
	state.direction = state.nextDirection;
	switch (state.direction)
	{
	case up:
		snake[0].y--;
		break;
	case down:
		snake[0].y++;
		break;
	case left:
		snake[0].x--;
		break;
	case right:
		snake[0].x++;
		break;
	}
	snake[0].x %= DISP_W;
	snake[0].y %= DISP_H;

	for(unsigned i = 1; i < state.length; i++)
	{
		if(snake[0].x == snake[i].x && snake[0].y == snake[i].y)
		{
			//whoops, game over dude
			return 0;
		}
	}

	unsigned food_consumed = 0;
	if(state.food.valid)
	{
		if(snake[0].x == state.food.x && snake[0].y == state.food.y)
		{
			//we snake. we schlängel. we eat.
			state.food.valid = 0;
			state.length++;		//this may read off-by-one in snake array.
			food_consumed = 1;
		}
	}

	fb_set_pixel_direct(snake[0].x, snake[0].y, 1);
	for(unsigned i = 1; i < state.length; i++)
	{
		struct Snakesegment buf2 = snake[i];
		snake[i] = buf;
		buf = buf2;		//wow, is it possible using more moves?
	}
	if(!food_consumed)
	{
		//last pixel gets deleted
		fb_set_pixel_direct(buf.x, buf.y, 0);
	}
	return 1;
}


void delayDifficulty()
{
	//Progress from 1 to 0
	float progress = ((float)(MAX_SNAKE_LENGTH - state.length))/MAX_SNAKE_LENGTH;
	if(state.intelligent)
		progress /= 2;
	sleep(1 + progress * MAX_DELAY);
}


int main (void)
{
	_init();
	//setup default global interrupt handler
	for (int gisr = 0; gisr < PLIC_NUM_INTERRUPTS; gisr++){
		g_ext_interrupt_handlers[gisr] = invalid_global_isr;
	}
	setup_button_irq();

	// Enable Global (PLIC) interrupts.
	set_csr(mie, MIP_MEIP);

	// Enable all interrupts
	set_csr(mstatus, MSTATUS_MIE);

	oled_init();
	fb_init();


	while (1) {
		reset_state();
		while(snake_step() && state.length < MAX_SNAKE_LENGTH)
		{
			delayDifficulty();
			if(!state.food.valid && (get_random_number() > 0x0F))
			{
				spawn_food();
			}
			if(state.intelligent)
				think();
		}
		fadeOut(1000);
		oled_clear();
		setContrast(255);

		printText("\n    GAME OVER\n");
		uint8_t buf[5];
		printText("\nScore: ");
		sprintf(buf, "%04u", state.length);
		printText(buf);
		sleep(5000);

		cls();
		fb_init();
		fb_flush();
	}
}
