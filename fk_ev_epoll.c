#include <sys/epoll.h>

/* 
 * I use emask to track the existing ev associated to fd because of 
 * the limitation of the interface of epoll, and I think it's better 
 * than using 2 array of fk_tmev pointer which used in libevent^_^
 */

typedef struct _fk_epoll {
	int efd;
	unsigned max_evs;
	struct epoll_event ev;/* temporary variable */
	struct epoll_event *evlist;
	char *emask;/* to track event associated to fd */
} fk_epoll;

static void *fk_epoll_create(unsigned max_files);
static int fk_epoll_add(void *ev_iompx, int fd, char type);
static int fk_epoll_remove(void *ev_iompx, int fd, char type);
static int fk_epoll_dispatch(void *ev_iompx, struct timeval *timeout);

fk_mpxop epoll_op = {
	"epoll",
	fk_epoll_create,
	fk_epoll_add,
	fk_epoll_remove,
	fk_epoll_dispatch
};

void *fk_epoll_create(unsigned max_files)
{
	int efd;
	fk_epoll *iompx;

	efd = epoll_create(1);
	if (efd < 0) {
		return NULL;
	}

	iompx = (fk_epoll *)fk_mem_alloc(sizeof(fk_epoll));

	iompx->max_evs = max_files;
	iompx->efd = efd;

	iompx->evlist = (struct epoll_event *)fk_mem_alloc(sizeof(struct epoll_event) * iompx->max_evs);
	iompx->emask = (char *)fk_mem_alloc(sizeof(char *) * iompx->max_evs);
	bzero(iompx->emask, sizeof(char *) * iompx->max_evs);

	return iompx;
}

int fk_epoll_add(void *ev_iompx, int fd, char type)
{
	int rt, op;
	fk_epoll *iompx;
	int32_t oev, nev;
	char otp;

	iompx = (fk_epoll *)ev_iompx;

	otp = iompx->emask[fd];
	if (type & otp) {
		fk_log_error("try to add existing ev\n");
		return FK_EV_ERR;
	}

	oev = 0x0000;
	if (otp & FK_IOEV_READ) {
		oev |= EPOLLIN;
	}
	if (otp & FK_IOEV_WRITE) {
		oev |= EPOLLOUT;
	}

	nev = 0x0000;
	if (type & FK_IOEV_READ) {
		nev |= EPOLLIN;
	}
	if (type & FK_IOEV_WRITE) {
		nev |= EPOLLOUT;
	}

	if (oev == 0x0000) {
		op = EPOLL_CTL_ADD;
	} else {
		op = EPOLL_CTL_MOD;
	}
	iompx->ev.events = oev | nev;
	iompx->ev.data.fd = fd;

	rt = epoll_ctl(iompx->efd, op, fd, &(iompx->ev));
	if (rt < 0) {
		fk_log_error("epoll_ctl failed: %s\n", strerror(errno));
		return FK_EV_ERR;
	}

	/* if succeed, save emask */
	iompx->emask[fd] = otp | type;
	return FK_EV_OK;
}

int fk_epoll_remove(void *ev_iompx, int fd, char type)
{
	int rt, op;
	fk_epoll *iompx;
	int32_t oev, nev;
	char otp;

	iompx = (fk_epoll *)ev_iompx;

	otp = iompx->emask[fd];
	if (!(type & otp)) {
		fk_log_error("try to remove a non-existing ev\n");
		return FK_EV_ERR;
	}

	oev = 0x0000;
	if (otp & FK_IOEV_READ) {
		oev |= EPOLLIN;
	}
	if (otp & FK_IOEV_WRITE) {
		oev |= EPOLLOUT;
	}

	nev = 0x0000;
	if (type & FK_IOEV_READ) {
		nev |= EPOLLIN;
	}
	if (type & FK_IOEV_WRITE) {
		nev |= EPOLLOUT;
	}

	if (oev == nev) {
		op = EPOLL_CTL_DEL;
	} else {
		op = EPOLL_CTL_MOD;
	}
	iompx->ev.events = oev & (~nev);
	iompx->ev.data.fd = fd;

	rt = epoll_ctl(iompx->efd, op, fd, &(iompx->ev));
	if (rt < 0) {
		fk_log_error("epoll_ctl failed: %s\n", strerror(errno));
		return FK_EV_ERR;
	}

	/* if succeed, remove from the emask */
	iompx->emask[fd] = otp & (~type);/* my clever!!!!!! */
	return FK_EV_OK;
}

int fk_epoll_dispatch(void *ev_iompx, struct timeval *timeout)
{
	char type;
	fk_epoll *iompx;
	int i, nfds, fd, ms_timeout;

	iompx = (fk_epoll *)ev_iompx;

	ms_timeout = -1;
	if (timeout != NULL) {
		ms_timeout = fk_util_tv2millis(timeout);
	}

	//fk_log_debug("epoll to wait\n");
	nfds = epoll_wait(iompx->efd, iompx->evlist, iompx->max_evs, ms_timeout);
	//fk_log_debug("epoll return\n");
	if (nfds < 0) {
		if (errno != EINTR) {
			return FK_EV_ERR;
		}
		return FK_EV_OK;
	}

	for (i = 0; i < nfds; i++) {
		fd = iompx->evlist[i].data.fd;
		type = 0x00;
		if (iompx->evlist[i].events & (EPOLLIN | EPOLLHUP | EPOLLERR)) {
			type |= FK_IOEV_READ;
		}
		if (iompx->evlist[i].events & (EPOLLOUT | EPOLLHUP | EPOLLERR)) {
			type |= FK_IOEV_WRITE;
		}
		fk_ev_activate_ioev(fd, type);
	}

	return FK_EV_OK;
}
