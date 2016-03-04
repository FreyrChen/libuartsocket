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
#include <fcntl.h>
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
	char *p;
	ssize_t len, size;

	fprintf(stderr, "Call callback 8800\n");
	while (1) {
		/** write */
		while ((len = sock_write(h, STRING0, strlen(STRING0), 0)) <= 0) {
			if (len == 0)
				continue;
			fprintf(stderr, "[cb_8800 %p] loop write(errno %d): %s\n", \
					cb_8800, \
					errno, \
					strerror(errno));
			return ((void *)0);
		}
		fprintf(stderr, "[cb_8800 %p] loop write(%ld): %s\n", \
				cb_8800, \
				len, \
				STRING0);

		/** read */
		while ((len = sock_read(h, buf, 100 - 1, 0)) <= 0) {
			if (len == 0)
				continue;
			fprintf(stderr, "[cb_8800 %p] loop read(errno %d): %s\n", \
					cb_8800, \
					errno, \
					strerror(errno));
			return ((void *)0);
		}
		fprintf(stderr, "[cb_8800 %p] loop read(%ld): %s\n", cb_8800, len, buf);
	}

	return (void *)0;
}

void *cb_8080(sock_handle_t *h, sock_rdwr_t *rdwr)
{
	sock_rdwr_t sock_read = rdwr[SOCK_READ_INDEX];
	sock_rdwr_t sock_write = rdwr[SOCK_WRITE_INDEX];

	char buf[100] = {0};
	ssize_t len;

	fprintf(stderr, "Call callback 8080\n");
	while (1) {
		/** write */
		while ((len = sock_write(h, STRING1, strlen(STRING1), 0)) <= 0) {
			if (len == 0)
				continue;
			fprintf(stderr, "[cb_8080 %p] loop write(errno %d): %s\n", \
					cb_8080, \
					errno, \
					strerror(errno));
			return ((void *)0);
		}
		fprintf(stderr, "[cb_8080 %p] loop write(%ld): %s\n", \
				cb_8080, \
				len, \
				STRING1);

		/** read */
		while ((len = sock_read(h, buf, 100 - 1, 0)) <= 0) {
			if (len == 0)
				continue;
			fprintf(stderr, "[cb_8080 %p] loop read(errno %d): %s\n", \
					cb_8080, \
					errno, \
					strerror(errno));
			return ((void *)0);
		}
		fprintf(stderr, "[cb_8080 %p] loop read(%ld): %s\n", cb_8080, len, buf);
	}

	return (void *)0;
}

int main(int argc, char *argv[])
{
	sock_handle_t *h0 = NULL, *h1 = NULL;

	sock_connect(&h0, IP0, PORT0, 0, cb_8800);
	sock_connect(&h1, IP1, PORT1, 0, cb_8080);
	while (!sock_alive(h0) || !sock_alive(h1))
		sleep(4);

	fprintf(stderr, "Main program exit\n");
	sock_free(&h0);
	sock_free(&h1);

	return 0;
}
