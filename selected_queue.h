#ifndef __FANFAN_SELECTED_QUEUE_H__
#define __FANFAN_SELECTED_QUEUE_H__

#include <deque>
#include <pthread.h>
#include <string>

using namespace std;

struct SSQItem
{
	string url;
	int fail_count;
	int dns_count;
	time_t last_crawl_time;
};

class CSelectedQueue
{
public:
    int init(void);
	int destroy(void);
	bool push(SSQItem qi);
	bool pop(SSQItem& qi);
	int size(void);
private:
    deque<SSQItem> m_queue;
	pthread_mutex_t m_mutex;
};

#endif
