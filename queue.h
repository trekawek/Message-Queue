#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MSG_QUEUE_SUCCESS  0 // When everything is OK.
#define MSG_QUEUE_TIMEOUT  1 // When timeout occurs.
#define MSG_QUEUE_STOPPED  2 // When send/recv is stopped by stop_signal.
#define MSG_QUEUE_BUSY     3 // When queue_destroy() is called on active queue.
#define MSG_QUEUE_TOOBIG   4 // When message is bigger than buffor in recv.
#define MSG_QUEUE_DISABLED 5 // When send/recv is called on queue that is being
                             // destroyed.

// Node struct (necessary to create message queue struct).
struct node
{
	int priority;
	void *buf;
	size_t buflen;
	struct node *next;
	int read;
};
typedef struct node node_t;

// Message queue struct.
struct msg_queue
{
	int length;
	int readers;
	int disabled;
	node_t *_queue;
	pthread_mutex_t mutex;
};
typedef struct msg_queue msg_queue_t;

// Create new message queue.
int queue_create(msg_queue_t *msg_queue);

// Send buf to the message queue. buflen is the size of the buffor. timeout
// is given is seconds (could be 0 for no timeout). If stop is not NULL and
// the pointed integer become 1 queue_send will stop and return
// MSG_QUEUE_STOPPED.
int queue_send(msg_queue_t *msg_queue, void *buf, size_t buflen, int priority,
               int timeout, int *stop);

// Get message from the message queue. Message is placed into buf. Maximum
// buffor size has to be placed in buflen. After successful return buflen
// will contain received message length. timeout and *stop like in the
// queue_send.
int queue_recv(msg_queue_t *msg_queue, void *buf, size_t *buflen,
               int *priority, int timeout, int *stop);

// Destroy given queue.
int queue_destroy(msg_queue_t *msg_queue);
