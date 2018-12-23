#include <c_types.h>
#include <eagle_soc.h> 
#include <gpio.h>
#include "debug.h"
#include "io_config.h"


/* GPIO */

// DATA PIN
#define DATA_MUX			PERIPHS_IO_MUX_MTCK_U	
#define DATA_NUM			13
#define DATA_FUNC			FUNC_GPIO13

// Clock PIN
#define CLOCK_MUX			PERIPHS_IO_MUX_MTMS_U	
#define CLOCK_NUM			14
#define CLOCK_FUNC			FUNC_GPIO14

#define DISPLAY_INTENSITY	1


#define GPIO_SET(n, v)	GPIO_OUTPUT_SET(GPIO_ID_PIN(n), v)
#define DATA_SET(v)		GPIO_OUTPUT_SET(GPIO_ID_PIN(DATA_NUM), v)
#define CLOCK_SET(v)	GPIO_OUTPUT_SET(GPIO_ID_PIN(CLOCK_NUM), v)
#define LOW				0
#define HIGH			1


LOCAL uint8_t display_buffer[8] = {0, 0, 0, 0, 0, 0, 0, 0};


void ICACHE_FLASH_ATTR
display_send(char data) {
	uint8_t i;
	for (i = 0; i < 8; i++) {
		CLOCK_SET(LOW);
		DATA_SET(data & 1 ? HIGH : LOW);
		data >>= 1;
		CLOCK_SET(HIGH);
	}
}



void ICACHE_FLASH_ATTR
display_send_command(char cmd) {
	DATA_SET(LOW);
	display_send(cmd);
	DATA_SET(HIGH);
}


void ICACHE_FLASH_ATTR
display_send_data(uint8_t address, char data) {
	display_send_command(0x44);
	DATA_SET(LOW);
	display_send(0xC0 | address);
	display_send(data);
	DATA_SET(HIGH);
}



void ICACHE_FLASH_ATTR
display_draw() {
	uint8_t i;
	for(i=0; i < 8; i++) {
		display_send_data(i, display_buffer[i]);
		DATA_SET(LOW);
		CLOCK_SET(LOW);
		CLOCK_SET(HIGH);
		DATA_SET(HIGH);
	}
	display_send_command(0x88 | DISPLAY_INTENSITY);
}


void ICACHE_FLASH_ATTR
display_clear() {
    uint8_t i;
	for(i=0; i < 8; i++) {
		display_buffer[i] = 0;
	}
}


void ICACHE_FLASH_ATTR
display_dot(uint8_t x, uint8_t y, bool on) {
	x &= 0x07;
	y &= 0x07;

	if (on) {
		display_buffer[y] |= 1 << x;
	}
	else {
		display_buffer[y] &= ~(1 << x);
	}
}


void ICACHE_FLASH_ATTR
display_init() {
	PIN_FUNC_SELECT(DATA_MUX, DATA_FUNC);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(DATA_NUM), 1);

	PIN_FUNC_SELECT(CLOCK_MUX, CLOCK_FUNC);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(CLOCK_NUM), 1);
}



LOCAL uint8_t digitmap[10][5] = {
	{7, 5, 5, 5, 7},  // 0
	{7, 2, 2, 3, 2},  // 1
	{7, 1, 7, 4, 7},  // 2
	{7, 4, 7, 4, 7},  // 3
	{4, 4, 7, 5, 5},  // 4
	{7, 4, 7, 1, 7},  // 5
	{7, 5, 7, 1, 7},  // 6
	{4, 4, 4, 4, 7},  // 7
	{7, 5, 7, 5, 7},  // 8
	{7, 4, 7, 5, 7},  // 9
};


void ICACHE_FLASH_ATTR
display_number(uint8_t n) {
	int i;
	if (n < 0 || n > 99) {
		return;
	}
	uint8_t d0 = n % 10;
	uint8_t d1 = n / 10;
	for (i = 0; i < 5; i++) {
		display_buffer[i] = (digitmap[d0][i] << 4) | (d1? digitmap[d1][i]: 0);
	}
}


LOCAL uint8_t charmap[26][5] = {
	{5, 5, 7, 5, 7},  // A
	{7, 5, 7, 1, 1},  // b
	{7, 1, 1, 1, 7},  // C
	{3, 5, 5, 5, 3},  // D
	{7, 1, 7, 1, 7},  // E
	{1, 1, 7, 1, 7},  // F
	{7, 4, 7, 5, 7},  // g
	{5, 5, 7, 5, 5},  // H
	{7, 2, 2, 2, 7},  // I
	{3, 4, 4, 4, 6},  // J
	{5, 3, 1, 3, 5},  // K
	{7, 1, 1, 1, 1},  // L
	{5, 5, 5, 7, 5},  // M
	{5, 5, 5, 5, 3},  // N
	{7, 5, 5, 5, 7},  // O
	{1, 1, 7, 5, 7},  // P
	{4, 4, 7, 5, 7},  // q
	{5, 3, 7, 5, 7},  // R
	{3, 4, 2, 1, 6},  // S
	{2, 2, 2, 2, 7},  // T
	{7, 5, 5, 5, 5},  // U
	{2, 5, 5, 5, 5},  // V
	{5, 7, 5, 5, 5},  // W
	{5, 5, 2, 5, 5},  // X
	{7, 4, 7, 5, 5},  // Y
	{7, 1, 2, 4, 7},  // Z
};


LOCAL uint8_t ICACHE_FLASH_ATTR
normalize_char(uint8_t c) {
	if (c >= 97 && c <= 122) {
		return c - 32;
	}
	return c;
}

	
void ICACHE_FLASH_ATTR
display_char(uint8_t c0, int pos) {
	int i;
	c0 = normalize_char(c0);
	if ((c0 < 65) || (c0 > 90) || (pos > 7) || (pos < -2)) {
		return;
	}
	uint8_t d0 = c0 - 65; 
	for (i = 0; i < 5; i++) {
		display_buffer[i] &= (0xFF << (pos+3)) | (0xFF >> (8-pos));
		display_buffer[i] |= (pos > 0) ? charmap[d0][i] << pos: \
				charmap[d0][i] >> abs(pos);
	}
}
