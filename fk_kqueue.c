#include <sys/event.h>

typedef struct _fk_kqueue {
	int kq;
	int max_evs;
	struct kevent kev;
	struct kevent *evlist;
	unsigned char *emask;
} fk_kqueue;

static void *fk_kqueue_create(int max_fds);
static int fk_kqueue_add(void *ev_iompx, int fd, unsigned char type);
static int fk_kqueue_remove(void *ev_iompx, int fd, unsigned char type);
static int fk_kqueue_dispatch(void *ev_iompx, struct timeval *timeout);

fk_mpxop kqueue_op = {
	fk_kqueue_create,
	fk_kqueue_add,
	fk_kqueue_remove,
	fk_kqueue_dispatch
};

void *fk_kqueue_create(int max_fds)
{
	int kq;
	fk_kqueue *iompx;

	kq = kqueue();
	if (kq < 0) {
		exit(0);
	}

	iompx = (fk_kqueue *)fk_mem_alloc(sizeof(fk_kqueue));
	iompx->max_evs = max_fds * 2;//read config from global setting
	iompx->kq = kq;
	iompx->evlist = (struct kevent *)fk_mem_alloc(sizeof(struct kevent) * iompx->max_evs);
	iompx->emask = (unsigned char *)fk_mem_alloc(sizeof(unsigned char *) * max_fds);
	bzero(iompx->emask, sizeof(unsigned char *) * max_fds);

	return iompx;
}

int fk_kqueue_add(void *ev_iompx, int fd, unsigned char type)
{
	int rt;
	fk_kqueue *iompx;
	unsigned char otyp;

	iompx = (fk_kqueue *)(ev_iompx);

	otyp = iompx->emask[fd];
	if (otyp & type) {
		return -1;
	}

	if (type & FK_EV_READ) {
		EV_SET(&(iompx->kev), fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL );
		rt = kevent(iompx->kq, &(iompx->kev), 1, NULL, 0, NULL);
		if (rt < 0) {
			fk_log_error("kevent add read failed: %s\n", strerror(errno));
			return -1;
		}
		iompx->emask[fd] |= FK_EV_READ;
	}

	if (type & FK_EV_WRITE) {//both read & write
		EV_SET(&(iompx->kev), fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL );
		rt = kevent(iompx->kq, &(iompx->kev), 1, NULL, 0, NULL);
		if (rt < 0) {
			fk_log_error("kevent add write failed: %s\n", strerror(errno));
			return -1;
		}
		iompx->emask[fd] |= FK_EV_WRITE;
	}

	return 0;
}

int fk_kqueue_remove(void *ev_iompx, int fd, unsigned char type)
{
	int rt;
	fk_kqueue *iompx;
	unsigned char otyp;

	iompx = (fk_kqueue *)ev_iompx;

	otyp = iompx->emask[fd];

	if (type & (~otyp)) {
		return -1;
	}

	if (type & FK_EV_READ) {
		EV_SET(&(iompx->kev), fd, EVFILT_READ, EV_DELETE, 0, 0, NULL );
		rt = kevent(iompx->kq, &(iompx->kev), 1, NULL, 0, NULL);
		if (rt < 0) {
			fk_log_error("kevent delete read failed: %s\n", strerror(errno));
			return -1;
		}
		iompx->emask[fd] &= (~FK_EV_READ);
	}
	if (type & FK_EV_WRITE) {//both read & write
		EV_SET(&(iompx->kev), fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL );
		rt = kevent(iompx->kq, &(iompx->kev), 1, NULL, 0, NULL);
		if (rt < 0) {
			fk_log_error("kevent delete write failed: %s\n", strerror(errno));
			return -1;
		}
		iompx->emask[fd] &= (~FK_EV_WRITE);
	}

	return 0;
}

int fk_kqueue_dispatch(void *ev_iompx, struct timeval *timeout)
{
	int i, fd, nfds;
	fk_kqueue *iompx;
	unsigned char type;
	struct timespec *pt;
	struct timespec kev_timeout;

	iompx = (fk_kqueue *)ev_iompx;

	pt = NULL;
	if (timeout != NULL) {
		FK_UTIL_TV2TS(timeout, &kev_timeout);
		pt = &kev_timeout;
	}

	//fk_log_debug("kevent to wait\n");
	nfds = kevent(iompx->kq, NULL, 0, iompx->evlist, iompx->max_evs, pt);
	if (nfds < 0) { }

	//fk_log_debug("kevent return\n");
	for (i = 0; i < nfds; i++) {
		fd = iompx->evlist[i].ident;
		if (iompx->evlist[i].filter == EVFILT_READ) {
			//fk_log_debug("EVFILT_READ\n");
			type = FK_EV_READ;
		}
		if (iompx->evlist[i].filter == EVFILT_WRITE) {
			//fk_log_debug("EVFILT_WRITE\n");
			type = FK_EV_WRITE;
		}
		//fk_log_debug("i: %d	fd: %lu	type: %d\n", i, iompx->evlist[i].ident, iompx->evlist[i].filter);
		fk_ev_ioev_activate(fd, type);
	}

	return 0;
}
