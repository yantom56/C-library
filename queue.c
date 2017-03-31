








///////////////////////////////////////////////////////////////////////////////
//////////// SYSTEM WIDE MESSAGE QUEUE
#include <mqueue.h>
#include <errno.h>
// MSGSIZE = 256
int gf_mq_send(char *name, char *msg, int len)
{
	// JOHN YAN - 2017/03/28 - LENGTH: 148
	// John Yan - 2017/03/28 - send message & start 3100
	mqd_t mqdes;   // queue descriptors
	struct mq_attr attr;
	attr.mq_flags = 0;	// BLOCK queue on purpose!
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = 256;
	attr.mq_curmsgs = 0;
	char qname[128];
	sprintf(qname, "/queue-%s", name);
	mode_t oldm = umask(0111);	// Without X
	if ((mqdes = mq_open(qname, O_WRONLY|O_CREAT, 0666, &attr)) == -1)
		dbg_msg(1, "mq_open error: %s", strerror(errno));
	umask(oldm);
	//////////////////
	// BLOCK mode send
	if (mq_send(mqdes, msg, len, 0) == -1)
		dbg_msg(1, "mq_send error: %s", strerror(errno));
	mq_close(mqdes);
	///////////////////////////////////////
	return 0;
}

///////////////////////////

/*
 * Singly-linked Tail queue declarations.
 */
#define	STAILQ_HEAD(name, type)						\
struct name {								\
	struct type *stqh_first;/* first element */			\
	struct type **stqh_last;/* addr of last next element */		\
}

#define	STAILQ_HEAD_INITIALIZER(head)					\
	{ NULL, &(head).stqh_first }

#define	STAILQ_ENTRY(type)						\
struct {								\
	struct type *stqe_next;	/* next element */			\
}

#define	STAILQ_NEXT(elm, field)	((elm)->field.stqe_next)

#define	STAILQ_INSERT_TAIL(head, elm, field) do {			\
	STAILQ_NEXT((elm), field) = NULL;				\
	*(head)->stqh_last = (elm);					\
	(head)->stqh_last = &STAILQ_NEXT((elm), field);			\
} while (0)


#define	STAILQ_FIRST(head)	((head)->stqh_first)

#define	STAILQ_REMOVE_HEAD(head, field) do {				\
	if ((STAILQ_FIRST((head)) =					\
	     STAILQ_NEXT(STAILQ_FIRST((head)), field)) == NULL)		\
		(head)->stqh_last = &STAILQ_FIRST((head));		\
} while (0)


///////////// private ring buffer
struct mq_msg_q {
	STAILQ_ENTRY(mq_msg_q) entry;
	char mq_msg_data[512];
};
static STAILQ_HEAD(, mq_msg_q) mq_msg_buf = STAILQ_HEAD_INITIALIZER(mq_msg_buf);
//
static int rb_push_tail(char *buf)
{
	struct mq_msg_q *temp;
	temp = (struct mq_msg_q *)malloc(sizeof(struct mq_msg_q));
	memcpy(temp->mq_msg_data, buf, 512);
	//
	STAILQ_INSERT_TAIL(&mq_msg_buf, temp, entry);
	return 0;
}
static int rb_pop_head(char *buf)
{
	struct mq_msg_q *temp;
	temp = STAILQ_FIRST(&mq_msg_buf);
	if (temp == NULL)
		return -1;
	STAILQ_REMOVE_HEAD(&mq_msg_buf, entry);
	//
	memcpy(buf, temp->mq_msg_data, 512);
	free(temp);
	return 0;
}
/// TIMEOUT: 3 seconds
int gf_mq_receive(char *name, char *msg)
{
	// John Yan - 2017/03/28 - send message & start 3100
	mqd_t mqdes;   // queue descriptors
	char qname[128];
	sprintf(qname, "/queue-%s", name);
#if 0
	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = 256;
	attr.mq_curmsgs = 0;
	mode_t oldm = umask(0111);	// Without X
	if ((mqdes = mq_open(qname, O_RDONLY|O_CREAT, 0666, &attr)) == -1)
		dbg_msg(1, "mq_open error: %s", strerror(errno));
	umask(oldm);
#else
	// SELECT MAY SLEEP!!
	// John Yan - 2017/03/31 - O_NONBLOCK mode
	if ((mqdes = mq_open(qname, O_RDONLY|O_NONBLOCK)) == -1) {
		dbg_msg(1, "mq_open O_NONBLOCK: %s", strerror(errno));
		return -1;
	}
	while (mq_receive(mqdes, msg, 512, 0) != -1)
		rb_push_tail(msg);
	//////////
	mq_close(mqdes);
	////////////
	// return one message if exist.
	if (rb_pop_head(msg) == 0)
		return 0;
#endif
	// return -1 OR wait ??
	// let's wait for another 3 seconds
	///////// Standalone logic
	/////////////////////////////////////////////////////////
	if ((mqdes = mq_open(qname, O_RDONLY)) == -1) {
		dbg_msg(1, "mq_open error: %s", strerror(errno));
		return -1;
	}
	///////////// Linux only
	fd_set rfds;
	struct timeval tv;
	FD_ZERO(&rfds);
	FD_SET(mqdes, &rfds);
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (select(mqdes+1, &rfds, NULL, NULL, &tv) > 0) {
		if (mq_receive(mqdes, msg, 512, 0) != -1) {
			mq_close(mqdes);
			return 0;	// success, return this msg
		}
	}
	// timeout or error
	if (errno != EAGAIN)
		dbg_msg(1, "gf_mq_receive: %s", strerror(errno));
	mq_close(mqdes);
	return -1;
#if 0
	/////////////////////
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 3;	// timeout: 3 seconds
	if (mq_timedreceive(mqdes, msg, 512, 0, &ts) != -1) {
		mq_close(mqdes);	// performance???
		return 0;	// success
	}
	// timeout or error
	if (errno != ETIMEDOUT)
		dbg_msg(1, "mq_timedreceive error: %s", strerror(errno));
	mq_close(mqdes);

	return -1;
#endif
}











