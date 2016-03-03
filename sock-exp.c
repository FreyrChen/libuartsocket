/*
 * sock-exp.c
 *
 *  Created on: Feb 29, 2016
 *      Author: yuyue <yuyue2200@hotmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "sock.h"

#define IP0 "127.0.0.1"
#define PORT0 "8800"
#define IP1 "127.0.0.1"
#define PORT1 "8080"

#define STRING0	"hello world in strings-0"
#define STRING1 "hello world in strings-1"

void *cb_8800(sock_handle_t *h, sock_rdwr_t *rdwr)
{
	sock_rdwr_t sock_read = rdwr[SOCK_READ_INDEX];
	sock_rdwr_t sock_write = rdwr[SOCK_WRITE_INDEX];

	char buf[100] = {0};
	ssize_t len;

	while (sock_write(h, STRING0, strlen(STRING0), 500) <= 0)
		;
	while ((len = sock_read(h, buf, 100, 500)) <= 0)
			;
	if (len > 0)
		printf("cb_8800 read: %s\n", buf);

	return (void *)0;
}

void *cb_8080(sock_handle_t *h, sock_rdwr_t *rdwr)
{
	sock_rdwr_t sock_read = rdwr[SOCK_READ_INDEX];
	sock_rdwr_t sock_write = rdwr[SOCK_WRITE_INDEX];

	char buf[100] = {0};
	ssize_t len;

	while (sock_write(h, STRING1, strlen(STRING1), 500) <= 0)
		;
	while ((len = sock_read(h, buf, 100, 500)) <= 0)
			;
	if (len > 0)
		printf("cb_8080 read: %s\n", buf);

	return (void *)0;
}

int main(int argc, char *argv[])
{
	sock_handle_t *h0 = NULL, *h1 = NULL;

	if (sock_connect(&h0, IP0, PORT0, 500, cb_8800) < 0) {
		printf("sock_connect(8800): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (sock_connect(&h1, IP1, PORT1, 500, cb_8080) < 0) {
		printf("sock_connect(8080): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	while (sock_alive(h0) || sock_alive(h1))
		;

	sock_free(&h0);
	sock_free(&h1);

	return 0;
}
