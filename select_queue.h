#ifndef __FANFAN_SELECTED_QUEUE_H__
#define __FANFAN_SELECTED_QUEUE_H__

#include <deque>

class CSelectedQueue
{
public:
    int init(int size);
	int destroy(void);
	bool push(SQueueItem qi);
	bool pop(SQueueItem& qi);
	bool size(void);
private:
    deque<SQueueItem> m_queue;
	pthread_mutex_t m_mutex;
};

#endif
