#include "selected_queue.h"

int CSelectedQueue::init(int size)
{
	m_size = size;
	m_count = 0;
	if (0 != pthread_mutex_init(&m_mutex, NULL))
	{
		return -1;
	}
	return 0;
}

int CSelectedQueue::destroy(void)
{
	if (0 != pthread_mutex_destroy(&m_mutex))
	{
		return -1;
	}
	return 0;
}

bool CSelectedQueue::push(SSQItem qi)
{
    pthread_mutex_lock(&m_mutex);
	if (m_count == m_size)  // 若队列已满，则push失败
	{
	    pthread_mutex_unlock(&m_mutex);
		return false;
	}
	m_count++;
	m_queue.push_back(qi);
	pthread_mutex_unlock(&m_mutex);

	return true;
}

bool CSelectedQueue::pop(SSQItem& qi)
{
	pthread_mutex_lock(&m_mutex);

	if (m_queue.empty())  // 若队列已空，则pop失败
	{
		pthread_mutex_unlock(&m_mutex);
		return false;
	}

	qi = m_queue.front();
	m_queue.pop_front();
	m_count--;
	pthread_mutex_unlock(&m_mutex);

	return true;
}

int CSelectedQueue::size(void)
{
	int size = 0;
	pthread_mutex_lock(&m_mutex);
	size = m_size;
	pthread_mutex_unlock(&m_mutex);

	return size;
}

int CSelectedQueue::avail_count(void)
{
	int c = 0;
	pthread_mutex_lock(&m_mutex);
	c = m_size-m_count;
	pthread_mutex_unlock(&m_mutex);

	return c;
}
