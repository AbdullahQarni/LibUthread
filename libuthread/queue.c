#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "queue.h"

// struct representing an entry in a queue linked list.
struct q_entry {
	// Generic pointer for the entry's "value."
	void *stored_data;
	// The next oldest and newest entries relative to the entry in the queue.
	struct q_entry *next;
	struct q_entry *prev;
};

// Using linked list implementation.
struct queue {
	int queue_length;
	struct q_entry *oldest;
	struct q_entry *newest;
};

queue_t queue_create(void)
{
	queue_t new_queue = malloc(sizeof(struct queue));
	if (new_queue == NULL) return NULL;

	// Initialize empty entries of the queue.
	new_queue->newest = NULL;
	new_queue->oldest = NULL;
	new_queue->queue_length = 0;
	return new_queue;
}

int queue_destroy(queue_t queue)
{
	if (queue == NULL || queue->queue_length) return -1;
	free(queue);
	return 0;
}

int queue_enqueue(queue_t queue, void *data)
{
	if (queue == NULL || data == NULL) return -1;

	// Allocate data for the new entry.
	struct q_entry *new_entry = malloc(sizeof(struct q_entry));
	if (new_entry == NULL) return -1;

	// Set backwards link between latest entry in queue and the new entry.
	new_entry->prev = queue->newest;
	// If the new entry is not the first entry, then set forward link.
	if (new_entry->prev != NULL) new_entry->prev->next = new_entry;

	// Since entry is newest, there are no entries after it.
	queue->newest = new_entry;
	new_entry->next = NULL;

	// If first entry in queue, it is the oldest and newest entry.
	if (queue->queue_length == 0) queue->oldest = new_entry;

	// Set entry's data address to requested data parameter.
	new_entry->stored_data = data;
	queue->queue_length++;

	return 0;
}

int queue_dequeue(queue_t queue, void **data)
{
	if (queue == NULL || data == NULL || queue->queue_length == 0)
		return -1;

	// Placeholder for the oldest entry so we can remember to free it later.
	struct q_entry *pop_entry = queue->oldest;

	// Set the read data to the address of the oldest entry's data.
	*data = queue->oldest->stored_data;

	// Pop the queue, setting the oldest entry to the second entry of the queue.
	queue->oldest = queue->oldest->next;
	// Make sure oldest item has nothing behind it.
	if (queue->oldest != NULL) queue->oldest->prev = NULL;
	// If list is now empty there are no newer items.
	else queue->newest = NULL;
	queue->queue_length--;

	// Deallocate the oldest entry.
	free(pop_entry);
	return 0;
}

int queue_delete(queue_t queue, void *data)
{
	if (queue == NULL || data == NULL) return -1;
	// Look for the data starting from the oldest entry.
	struct q_entry *iter_entry = queue->oldest;
	while(iter_entry != NULL)
	{
		if (iter_entry->stored_data == data)
		{
			if (iter_entry == queue->oldest)
			{
				// Oldest entry, mark next entry as oldest.
				queue->oldest = iter_entry->next;
				if (queue->oldest != NULL) queue->oldest->prev = NULL;
				else queue->newest = NULL;
			} else if (iter_entry == queue->newest)
			{
				// Newest entry, mark previous entry as newest.
				queue->newest = iter_entry->prev;
				queue->newest->next = NULL;
			} else {
				// Middle entry, bridge the gap between the next and previous entries.
				iter_entry->prev->next = iter_entry->next;
				iter_entry->next->prev = iter_entry->prev;
			}

			// Delete the entry.
			free(iter_entry);
			queue->queue_length--;
			return 0;
		}
		iter_entry = iter_entry->next;
	}
	// The loop was exited because we didn't find the data among the queue entries.
	return -1;
}

int queue_iterate(queue_t queue, queue_func_t func, void *arg, void **data)
{
	// Uninitialized queue or invalid function.
	if (queue == NULL || func == NULL) return -1;
	struct q_entry *iter_entry = queue->oldest;
	struct q_entry *iter_next = NULL;
	void *save_data = NULL;
	while (iter_entry != NULL)
	{
		// Store the next entry to iterate in case the current gets deleted.
		iter_next = iter_entry->next;
		// Store the current entry's data in case we return 1 following a deletion.
		save_data = iter_entry->stored_data;
		// If callback function returns 1, set address of data argument to address of current entry's data.
		if (func(queue, iter_entry->stored_data, arg) == 1)
		{
			if (data != NULL) *data = save_data;
			break;
		}
		iter_entry = iter_next;
	}
	return 0;
}

int queue_length(queue_t queue)
{
	if (queue == NULL) return -1;
	return queue->queue_length;
}

