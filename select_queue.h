#ifndef __FANFAN_SELECTED_QUEUE_H__
#define __FANFAN_SELECTED_QUEUE_H__

class CSelectedQueue
{
public:
    int init(int size);
	int destroy(void);
	bool push(SQueueItem qi);
	bool pop(SQueueItem* pqi);
	bool size(void);
private:
    int m_size;
    deque<SQueueItem> m_queue;
	pthread_mutex_t m_mutex;
};

#endif
