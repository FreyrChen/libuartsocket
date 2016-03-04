/*
 * sock.h
 *
 *  Created on: Feb 29, 2016
 *      Author: yuyue <yuyue2200@hotmail.com>
 */

#ifndef __SOCK_H__
#define __SOCK_H__
enum {
	MAX_CONN_TIMEOUT = 75 * 1000,
#define MAX_CONN_TIMEOUT MAX_CONN_TIMEOUT
};

enum {
	SOCK_READ_INDEX = 0,
#define SOCK_READ_INDEX SOCK_READ_INDEX
	SOCK_WRITE_INDEX = 1,
#define SOCK_WRITE_INDEX SOCK_WRITE_INDEX
	SOCK_MAX_INDEX = 2,
#define SOCK_MAX_INDEX SOCK_MAX_INDEX
};

/**
 * This structure describes socket handle.
 *
 * sock_handle_t is typically allocated once and then reused multiple times
 * to connect different peer host.
 * sock_handle_t MUST be allocated using sock_new() and MUST be freed with
 * sock_free().
 * @warning sizeof(sock_handle_t) is not a part of the public ABI.
 */
typedef struct sock_handle_t sock_handle_t;
typedef ssize_t (*sock_rdwr_t)(sock_handle_t *, void *, size_t, int);

/**
 * @WARNING Callback is a functions of thread environment.
 *          MUST be returned normally
 */
typedef void *(*sock_callback_t)(sock_handle_t *, sock_rdwr_t []);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Connect to the host:port and waiting for @timeout milliseconds of times
 *
 * @h		socket handle.
 * @host	host name or IP address.
 * @port	service or port.
 * @timeout waitting for connect to the host(MAX 75s, unit: milliseconds).
 *
 * @return  0 on success, < 0 on failure and errno is set appropriately.
 */
int sock_connect(sock_handle_t **h, const char *host, const char *serv, int timeout, sock_callback_t callback);

/**
 * Close the socket connections of socket handle and free memory block.
 *
 * @h        socket handle
 */
void sock_free(sock_handle_t **h);

/**
 * Find socket handle status.
 *
 * @h        socket handle
 *
 * @return   0, connection is closed, > 0 connect is online.
 */
int sock_alive(sock_handle_t *h);

#ifdef __cplusplus
}
#endif
#endif /* NET_H_ */
