#include "selected_queue.h"

int CSelectedQueue::init(void)
{
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
	m_queue.push_back(qi);
	pthread_mutex_unlock(&m_mutex);

	return true;
}

bool CSelectedQueue::pop(SSQItem& qi)
{
	pthread_mutex_lock(&m_mutex);

	if (m_queue.empty())
	{
		pthread_mutex_unlock(&m_mutex);
		return false;
	}

	qi = m_queue.front();
	m_queue.pop_front();
	pthread_mutex_unlock(&m_mutex);

	return true;
}

int CSelectedQueue::size(void)
{
	int size = 0;

	pthread_mutex_lock(&m_mutex);
	size = m_queue.size();
	pthread_mutex_unlock(&m_mutex);

	return size;
}
