#include "queue.h"

// Add the new element to the priority queue. If _queue == NULL then create
// priority queue. Function returns pointer to the head of the queue and sets
// unique node id if *id != NULL. If allocating memory fails it will return
// NULL.
node_t *_add(node_t *_queue, void *buf, int buflen, int priority, node_t **new_node)
{
	node_t *node, *cur_node, *prev_node = NULL;
	if(!(node = malloc(sizeof(node_t))))
		return NULL;
	if(new_node != NULL)
		*new_node = node;
	node->buf = malloc(buflen);
	memcpy(node->buf, buf, buflen);
	node->buflen = buflen;
	node->priority = priority;
	node->next = NULL;
	node->read = 0;
	for(cur_node = _queue; cur_node != NULL; cur_node = cur_node->next)
	{
		if(cur_node->priority > priority)
		{
			node->next = cur_node;
			if(prev_node)
				prev_node->next = node;
			else
				_queue = node;
			return _queue;
		}
		prev_node = cur_node;
	}
	if(prev_node) // tail
		prev_node->next = node;
	else // head
		_queue = node;
	return _queue;
}

// Copy node buffer to the given buffer with length buflen. After return buflen
// contains length of the data from node. Return MSG_QUEUE_SUCCESS if success
// or MSG_QUEUE_TOOBIG if node is bigger than buf.
int _copy_to_buf(node_t *node, void *buf, size_t *buflen)
{
	if(node->buflen > *buflen)
		return MSG_QUEUE_TOOBIG;
	memcpy(buf, node->buf, node->buflen);
	*buflen = node->buflen;
	return MSG_QUEUE_SUCCESS;
}

// Get unread element with the smallest priority.
node_t *_get_first(node_t *_queue)
{
	node_t *cur_node;
	for(cur_node = _queue; cur_node != NULL; cur_node = cur_node->next)
		if(!cur_node->read)
			return cur_node;
	return NULL;
}

// Delete from the priority queue given element. Return pointer to
// the head of the queue.
node_t *_delete(node_t *_queue, node_t *target_node)
{
	node_t *cur_node, *prev_node = NULL;
	for(cur_node = _queue; cur_node != NULL; cur_node = cur_node->next)
		if(cur_node == target_node)
		{
			if(prev_node)
				prev_node->next = cur_node->next;
			else
				_queue = cur_node->next;
			free(cur_node->buf);
			free(cur_node);
			return _queue;
		}
	return _queue;
}

int queue_create(msg_queue_t *new_queue)
{
	new_queue->length = 0;
	new_queue->readers = 0;
	new_queue->disabled = 0;
	new_queue->_queue = NULL;
	pthread_mutex_init(&new_queue->mutex, NULL);
	return MSG_QUEUE_SUCCESS;
}

int queue_send(msg_queue_t *msg_queue, void *buf, size_t buflen, int priority, int timeout, int *stop_signal)
{
	int result = -1, disabled = 0;
	node_t *new_node;
	clock_t stop;
	stop = clock() + timeout * CLOCKS_PER_SEC;

	// Check if the queue is not being destroyed.
	if(msg_queue->disabled)
		return MSG_QUEUE_DISABLED;

	pthread_mutex_lock(&msg_queue->mutex);
	// It's possible that in the above if(msg_queue->disabled) disabled==0
	// but when we were waiting for mutex, the queue_destroy function
	// was called and disabled==1 now. So let's check again.
	if(msg_queue->disabled)
	{
		pthread_mutex_unlock(&msg_queue->mutex);
		return MSG_QUEUE_DISABLED;
	}
	msg_queue->length++;
	msg_queue->_queue = _add(msg_queue->_queue, buf, buflen, priority, &new_node);
	pthread_mutex_unlock(&msg_queue->mutex);

	do
	{
		pthread_mutex_lock(&msg_queue->mutex);
		if(new_node->read)
			result = MSG_QUEUE_SUCCESS;
		else if(timeout != 0 && clock() >= stop)
			result = MSG_QUEUE_TIMEOUT;
		else if(stop_signal != NULL && *stop_signal)
			result = MSG_QUEUE_STOPPED;
		pthread_mutex_unlock(&msg_queue->mutex);
	} while(result == -1);

	pthread_mutex_lock(&msg_queue->mutex);
	if(result != MSG_QUEUE_SUCCESS)
		msg_queue->length--;
	msg_queue->_queue = _delete(msg_queue->_queue, new_node);
	pthread_mutex_unlock(&msg_queue->mutex);
	return result;
}

int queue_recv(msg_queue_t *msg_queue, void *buf, size_t *buflen, int *priority, int timeout, int *stop_signal)
{
	int result = -1;
	node_t *node;
	clock_t stop;
	stop = clock() + timeout * CLOCKS_PER_SEC;

	// See the queue_send function.
	if(msg_queue->disabled)
		return MSG_QUEUE_DISABLED;
	pthread_mutex_lock(&msg_queue->mutex);
	if(msg_queue->disabled)
	{
		pthread_mutex_unlock(&msg_queue->mutex);
		return MSG_QUEUE_DISABLED;
	}
	msg_queue->readers++;
	pthread_mutex_unlock(&msg_queue->mutex);
	do
	{
		pthread_mutex_lock(&msg_queue->mutex);
		if(msg_queue->length > 0)
		{
			node = _get_first(msg_queue->_queue);
			node->read = 1;
			msg_queue->length--;
			result = _copy_to_buf(node, buf, buflen);
			*priority = node->priority;
		}
		else if(timeout != 0 && clock() >= stop)
			result = MSG_QUEUE_TIMEOUT;
		else if(stop_signal != NULL && *stop_signal)
			result = MSG_QUEUE_STOPPED;
		pthread_mutex_unlock(&msg_queue->mutex);
	} while(result == -1);
	pthread_mutex_lock(&msg_queue->mutex);
	msg_queue->readers--;
	pthread_mutex_unlock(&msg_queue->mutex);
	return result;
}

int queue_destroy(msg_queue_t *msg_queue)
{
	node_t *cur_node = msg_queue->_queue, *next_node;
	int busy = 0;
	pthread_mutex_lock(&msg_queue->mutex);
	busy = (msg_queue->readers || msg_queue->length);
	if(!busy)
		msg_queue->disabled = 1;
	pthread_mutex_unlock(&msg_queue->mutex);

	if(busy)
		return MSG_QUEUE_BUSY;
	pthread_mutex_destroy(&msg_queue->mutex);

	while(cur_node)
	{
		next_node = cur_node->next;
		free(cur_node->buf);
		free(cur_node);
		cur_node = next_node;
	}
	return MSG_QUEUE_SUCCESS;
}
