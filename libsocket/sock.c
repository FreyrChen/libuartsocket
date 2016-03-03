/*
 * sock.c
 *
 *  Created on: Feb 29, 2016
 *      Author: yuyue <yuyue2200@hotmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "sock.h"

struct sock_handle_t {
	int fd;
	pthread_t tid;
	const char *host;
	const char *serv;
	sock_callback_t callback;
	sock_rdwr_t rdwr[SOCK_MAX_INDEX];
};

typedef struct epoll_handle_t {
	int epollfd;
	struct epoll_event ev;
	pthread_mutex_t lock;
	int nb_fds;
} epoll_handle_t;
static epoll_handle_t global_epoll = {-1, {0}, {0}, 0};

static int fd_set_nonblock(int fd, int enable)
{
	if (enable)
		return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
	else
		return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & (~O_NONBLOCK));
}

static int sock_init_addr_by_host_serv(struct sockaddr *addr, const char *host, const char *serv)
{
	struct addrinfo hints, *aip, *ailist;
	int fd = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = host ? 0 : AI_PASSIVE;

	if (getaddrinfo(host, serv, &hints, &ailist) < 0)
		return -1;
	for (aip = ailist; aip; aip = aip->ai_next) {
		fd = socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol);
		if (fd >= 0) {
			memcpy(addr, aip->ai_addr, sizeof(*addr));
			break;
		}
	}
	freeaddrinfo(ailist);

	return fd;
}

static sock_handle_t *sock_new(void)
{
	sock_handle_t *hd = NULL;

	hd = malloc(sizeof(*hd));
	if (hd) {
		memset(hd, 0, sizeof(*hd));
		hd->fd   = -1;
	}

	return hd;
}

static ssize_t sock_asyn_read(sock_handle_t *h, void *buf, size_t count, int timeout)
{
	int nfds;
	struct epoll_event *rev;

	rev = malloc(sizeof(*rev) * global_epoll.nb_fds);
	if (!rev)
		return -1;
	pthread_mutex_lock(&global_epoll.lock);
	nfds = epoll_wait(global_epoll.epollfd, rev, global_epoll.nb_fds, timeout);
	pthread_mutex_unlock(&global_epoll.lock);
	if (nfds > 0) {
		int n;
		for (n = 0; n < nfds; n++) {
			if ((rev[n].events & EPOLLOUT) && \
				(((sock_handle_t *)rev[n].data.u64)->fd == h->fd)) {
				ssize_t len;
				free(rev);
				len = recv(h->fd, buf, count, 0);
				printf("[fd %d]read size: %ld", h->fd, len);
				if (len < 0)
					printf(" [%d][%s]\n", errno, strerror(errno));
				if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
					len = 0;
				return len;
			}
		}
	}
	free(rev);
	return nfds ? -1 : 0;
}

static ssize_t sock_asyn_write(sock_handle_t *h, void *buf, size_t count, int timeout)
{
	int nfds;
	struct epoll_event *rev;

	rev = malloc(sizeof(*rev) * global_epoll.nb_fds);
	if (!rev)
		return -1;
	pthread_mutex_lock(&global_epoll.lock);
	nfds = epoll_wait(global_epoll.epollfd, rev, global_epoll.nb_fds, timeout);
	pthread_mutex_unlock(&global_epoll.lock);
	if (nfds > 0) {
		int n;
		for (n = 0; n < nfds; n++) {
			if ((rev[n].events & EPOLLOUT) && \
				(((sock_handle_t *)rev[n].data.u64)->fd == h->fd)) {
				ssize_t len;
				free(rev);
				len =  send(h->fd, buf, count, 0);
				printf("[fd %d]write size: %ld\n", h->fd, len);
				if (len < 0)
					printf(" [%d][%s]\n", errno, strerror(errno));
				if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
					len = 0;
				return len;
			}
		}
	}
	free(rev);
	return nfds ? -1 : 0;
}

static int sock_close(sock_handle_t *h)
{
	if (h->fd > 0)
		close(h->fd);
	free((void *)h->host);
	free((void *)h->serv);
	memset(h, 0, sizeof(*h));
	h->fd = -1;

	return 0;
}

void sock_free(sock_handle_t **h)
{
	sock_handle_t *hd = *h;

	if (hd->fd >= 0)
		sock_close(hd);
	free(hd);
	*h = NULL;
}

int sock_alive(sock_handle_t *h)
{
	return h->fd < 0 ? 0 : 1;
}

static void *sock_callback(void *args)
{
	sock_handle_t *hd = (sock_handle_t *)args;
	void *ret;

	ret = hd->callback(hd, hd->rdwr);
	epoll_ctl(global_epoll.epollfd, EPOLL_CTL_DEL, hd->fd, NULL);
	global_epoll.nb_fds--;
	sock_close(hd);

	return ret;
}

int sock_connect(sock_handle_t **h, const char *host, const char *serv, int timeout, sock_callback_t callback)
{
	struct sockaddr addr;
	sock_handle_t *hd = NULL;

	if (!host || !serv || !callback) {
		errno = EINVAL;
		return -1;
	}
	hd = *h ? *h : sock_new();
	if (!hd)
		return -1;
	hd->fd = sock_init_addr_by_host_serv(&addr, host, serv);
	if (hd->fd < 0)
		return -1;
	fd_set_nonblock(hd->fd, 1);
	setsockopt(hd->fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	if (connect(hd->fd, &addr, sizeof(addr)) < 0) {
		if (errno != EINPROGRESS) {
			close(hd->fd);
			hd->fd = -1;
			return -1;
		}
	}
	hd->callback               = callback;
	hd->host                   = strdup(host);
	hd->serv                   = strdup(serv);
	hd->rdwr[SOCK_READ_INDEX]  = sock_asyn_read;
	hd->rdwr[SOCK_WRITE_INDEX] = sock_asyn_write;
	if (global_epoll.epollfd < 0) {
		/** Don't mind parameter values of epoll_create() */
		global_epoll.epollfd = epoll_create(1);
		pthread_mutex_init(&global_epoll.lock, NULL);
	}
	if (timeout > MAX_CONN_TIMEOUT)
		timeout = MAX_CONN_TIMEOUT;
	global_epoll.ev.events   = EPOLLIN | EPOLLOUT;
	global_epoll.ev.data.u64 = (uint64_t)hd;
	if (epoll_ctl(global_epoll.epollfd, EPOLL_CTL_ADD, hd->fd, &global_epoll.ev))
		goto failure;
	global_epoll.nb_fds++;
	if (!sock_asyn_write(hd, (char [1]){0}, 0, 100)) {
		pthread_attr_t attr;
		int err;
		if (!(err = pthread_attr_init(&attr))) {
			if (!(err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))) {
				err = pthread_create(&hd->tid, NULL, &sock_callback, (void *)hd);
			}
			pthread_attr_destroy(&attr);
		}
		if (!err) {
			if (!*h)
				*h = hd;
			return 0;
		} else {
			errno = err;
		}
	}
failure:
	(!*h) ? sock_free(&hd) : sock_close(hd);

	return -1;
}
