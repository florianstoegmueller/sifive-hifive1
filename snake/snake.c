/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <metal/init.h>

#include "drivers.h"
#include "helpers.h"
#include "oled_shield.h"
#include "font.h"
#include "display.h"
#include "framebuffer.h"

#define INT_GPIO_BASE 8 // TODO

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

struct metal_interrupt *plic;

void button_handler(int id, void *data)
{
    id -= INT_GPIO_BASE;
	if(id ==  mapPinToReg(BUTTON_D))
	{
		if(state.direction != up)
			state.nextDirection = down;
	}
	else if(id == mapPinToReg(BUTTON_U))
	{
		if(state.direction != down)
			state.nextDirection = up;
	}
	else if(id == mapPinToReg(BUTTON_L))
	{
		if(state.direction != right)
			state.nextDirection = left;
	}
	else if(id == mapPinToReg(BUTTON_R))
	{
		if(state.direction != left)
			state.nextDirection = right;
	}
	else if(id == mapPinToReg(BUTTON_A))
	{
		state.intelligent ^= 1;
	}
	else
	{
	}
    metal_gpio_clear_interrupt(get_gpio(), id, METAL_GPIO_INT_FALLING);
}

int setup_button_irq()
{
    int rc, id;

    plic = metal_interrupt_get_controller(METAL_PLIC_CONTROLLER, 0);
    metal_interrupt_init(plic);

	for(unsigned i = 0; i < sizeof(buttons); i++)
	{
        id = INT_GPIO_BASE + mapPinToReg(buttons[i]);
		setPinInputPullup(buttons[i], 1);
		//enableInterrupt(buttons[i], 0);	//rise necessary?
		enableInterrupt(buttons[i], 1);	//fall
        rc = metal_interrupt_register_handler(plic, id, button_handler, 0);
        if (rc < 0) {
            printf("interrupt handler registration failed\n");
            return 0;
        }
        metal_interrupt_set_priority(plic, id, 2+i);
        if (metal_interrupt_enable(plic, id) == -1) {
            printf("interrupt enable failed\n");
            return 0;
        }
	    printf("Inited button %d\r\n", buttons[i]);
	}
	return 1;
}

uint8_t get_random_number()
{
	static uint8_t rand = 15;	//generated by fair dice roll
	rand += metal_cpu_get_mtime(get_cpu());
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
		if(abs(diffy) > DISP_H/2)
			diffy *= -1;
		if(abs(diffx) > DISP_W/2)
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
			future.x %= DISP_W;
			future.y %= DISP_H;

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
	state.intelligent = 0;
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

int main() {
    metal_init();
    if(!init_drivers())
        return 1;

    if(!setup_button_irq())
        return 1;

    if (metal_interrupt_enable(get_cpu_intr(), 0) == -1) {
        printf("Failed to enable CPU interrupt\n");
        return 1;
    }

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

    return 0;
}
