#ifndef __FANFAN_SELECTED_QUEUE_H__
#define __FANFAN_SELECTED_QUEUE_H__

#include <deque>
#include <pthread.h>
#include <string>

using namespace std;

typedef enum queue_type_t
{
	QUEUE_TYPE_CPQ = 1,  // cate优先队列
	QUEUE_TYPE_COQ = 2,  // cate普通队列
	QUEUE_TYPE_IPQ = 3,  // item 优先队列
	QUEUE_TYPE_IOQ = 4   // item 普通队列
};

struct SSQItem
{
	string url;
	int fail_count;
	int dns_count;
	time_t last_crawl_time;
	queue_type_t which_queue;
};

class CSelectedQueue
{
public:
    int init(int size);
	int destroy(void);
	bool push(SSQItem qi);
	bool pop(SSQItem& qi);
	int size(void);
    int avail_count(void);
private:
    deque<SSQItem> m_queue;
	pthread_mutex_t m_mutex;

	int m_size;
	int m_count;
};

#endif
