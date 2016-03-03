/*
 * serial-exp.c
 *
 *  Created on: Mar 2, 2016
 *      Author: yuyue <yuyue2200@hotmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "serial.h"

#define DEV      "/dev/ttyUSB0"
#define BAUDRATE 57600
#define OPTS     "8n1"

#define STRING0	"hello world in strings-0"
#define STRING1 "hello world in strings-1"

#define print_error(msg) \
	do { printf(msg "%s\n", strerror(errno)); exit(EXIT_FAILURE);} while (0)

void *cb_ttyusb0(serial_handle_t *h, serial_rdwr_t *rdwr)
{
	serial_rdwr_t serial_read = rdwr[SERIAL_READ_INDEX];
	serial_rdwr_t serial_write = rdwr[SERIAL_WRITE_INDEX];

	char buf[100] = {0};
	ssize_t len;

	while ((len = serial_read(h, buf, 100, 500)) <= 0)
			;
	if (len > 0)
		printf("cb_%s read: %s\n", DEV, buf);

	return (void *)0;
}

int main(void)
{
	serial_handle_t *s0;

	if (serial_open(&s0, DEV, BAUDRATE, OPTS, cb_ttyusb0) < 0) {
		printf("serial open(%s): %s\n", DEV, strerror(errno));
		exit(EXIT_FAILURE);
	}

	while (serial_alive(s0))
			;
	serial_free(&s0);
	return 0;
}
