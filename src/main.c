/*
 * Serial nonce verification with Teensy USB
 *
 * Copyright (C) 2013 Johannes Goetzfried <johannes@jgoetzfried.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307 USA.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include "usb_serial.h"

#define NONCESIZE 64

#define LED_CONFIG	(DDRD |= (1<<6))
#define LED_ON		(PORTD &= ~(1<<6))
#define LED_OFF		(PORTD |= (1<<6))
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

#define BLINK(delay)      \
	LED_ON;           \
	_delay_ms(delay); \
	LED_OFF;          \
	_delay_ms(delay); \
	LED_ON;           \
	_delay_ms(delay); \
	LED_OFF;          \
	_delay_ms(delay); \
	LED_ON;           \
	_delay_ms(delay); \
	LED_OFF;

static uint8_t g_ee_nonce[NONCESIZE] EEMEM = {
	'4', '2', '4', '2', '4', '2', '4', '2',
	'4', '2', '4', '2', '4', '2', '4', '2',
	'4', '2', '4', '2', '4', '2', '4', '2',
	'4', '2', '4', '2', '4', '2', '4', '2',
	'4', '2', '4', '2', '4', '2', '4', '2',
	'4', '2', '4', '2', '4', '2', '4', '2',
	'4', '2', '4', '2', '4', '2', '4', '2',
	'4', '2', '4', '2', '4', '2', '4', '2' };


static void send_str(const char *s);
static uint8_t recv_str(char *buf, uint8_t size);
static void do_command(char *buf, uint8_t n);

static uint8_t g_nonce[NONCESIZE];
static uint8_t g_verifycnt = 0;
static uint8_t g_verified = 0;

int main(void)
{
	char buf[128];
	uint8_t n;

	CPU_PRESCALE(0);

	usb_init();
	while (!usb_configured()) /* wait */ ;

	/* led blink; waiting time is mandatory */
	LED_CONFIG;
	BLINK(200);

	// read nonce from eeprom
	eeprom_read_block(g_nonce, g_ee_nonce, NONCESIZE);

	// wait for the user to run their terminal emulator program
	// which sets DTR to indicate it is ready to receive.
	while (!(usb_serial_get_control() & USB_SERIAL_DTR)) /* wait */ ;
	usb_serial_flush_input();

	send_str(PSTR("200 Welcome\r\n"));

	while (1) {
		n = recv_str(buf, sizeof(buf));
		if (g_verifycnt < 3)
			do_command(buf, n);
		else
			send_str(PSTR("404 Number of tries exceeded\r\n"));
	}
}


static void send_str(const char *s)
{
	char c;
	while (1) {
		c = pgm_read_byte(s++);
		if (!c) break;
		usb_serial_putchar(c);
	}
}

static uint8_t recv_str(char *buf, uint8_t size)
{
	int16_t r;
	uint8_t count = 0;

	while (count < size) {
		r = usb_serial_getchar();
		if (r != -1) {
			if (r == '\r' || r == '\n') return count;
			if (r >= ' ' && r <= '~') {
				*buf++ = r;
				count++;
			}
		}
	}
	return count;
}

/* command syntax: command<space>operand\n
 * commands:
 *   verify <nonce>
 *   store <nonce>
 * <nonce> is exactly NONCESIZE chars
 */
static void do_command(char *buf, uint8_t n)
{
	if (!strncmp_P(buf, PSTR("verify"), 6) && g_verified == 0) {
		if (!memcmp(buf + 7, g_nonce, NONCESIZE)) {
			g_verified = 1;
			LED_ON;
			send_str(PSTR("201 Verified\r\n"));
		}
		else {
			g_verifycnt++;
			BLINK(100);
			send_str(PSTR("401 Verification failed\r\n"));
		}
	}
	else if (!strncmp_P(buf, PSTR("store"), 5)) {
		if (g_verified == 0) {
			send_str(PSTR("402 Not verified\r\n"));
		}
		else if (n < 6 + NONCESIZE) {
			send_str(PSTR("403 Invalid nonce\r\n"));
		}
		else {
			eeprom_write_block(buf + 6, g_ee_nonce, NONCESIZE);
			send_str(PSTR("202 Nonce written\r\n"));
		}
	}
	else {
		send_str(PSTR("400 Unknown command\r\n"));
	}
}

