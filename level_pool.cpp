#include "level_pool.h"
#include <time.h>
#include <sys/time.h>

class SpiderConf;

int CLevelPool::init(CSpiderConf* conf)
{
	int ret = -1;
	mp_conf = conf;
	if ((ret=pthread_mutex_init(&m_mutex, NULL)) != 0)
	{
		return -1;
	}
	return 0;
}

int CLevelPool::destroy(void)
{
	int ret = pthread_mutex_destroy(&m_mutex);

	return 0==ret ? 0 : -1;
}

void CLevelPool::insert_level_info(string& site, SLevelInfo li)
{
	m_pool.insert(make_pair(site, li));
}

bool CLevelPool::is_crawl_enabled(string& site)
{
	CSpiderConf& conf = *mp_conf;

    pthread_mutex_lock(&m_mutex);
    time_t now = get_time_now();

	map<string, SLevelInfo>::iterator it = m_pool.find(site);
	if (it == m_pool.end())
	{
		SLevelInfo li;
		li.max_crawl_thread_count = conf.default_max_concurrent_thread_count;
		li.crawl_thread_count = 1;
		li.last_crawl_time = now;
		li.crawl_interval = conf.default_site_crawl_interval;
		m_pool.insert(make_pair(site, li));

		pthread_mutex_unlock(&m_mutex);
		return true;
	}
	if (it->second.crawl_thread_count >= conf.default_max_concurrent_thread_count) // thread concurrent control
	{
		pthread_mutex_unlock(&m_mutex);
		return false;
	}

    if (now - it->second.last_crawl_time < conf.default_site_crawl_interval) // site crawl interval control
	{
		pthread_mutex_unlock(&m_mutex);
		return false;
	}
	it->second.last_crawl_time = now;
	it->second.crawl_thread_count++;
	pthread_mutex_unlock(&m_mutex);
	return true;
}

void CLevelPool::finish_crawl(string& site)
{

	pthread_mutex_lock(&m_mutex);

    //time_t now = get_time_now();

	map<string, SLevelInfo>::iterator it = m_pool.find(site);
	if (it == m_pool.end())
	{
		pthread_mutex_unlock(&m_mutex);
		return;
	}
	//it->second.last_crawl_time = now;
	it->second.crawl_thread_count--;
	pthread_mutex_unlock(&m_mutex);
	return;
}

time_t CLevelPool::get_time_now(void)
{
	struct timeval tm;
	gettimeofday(&tm, NULL);

	return tm.tv_sec*1000 + tm.tv_usec/1000;
}

void CLevelPool::print(void)
{
	map<string, SLevelInfo>::iterator it = m_pool.begin();

	for (; it != m_pool.end(); it++)
	{
		printf("%s\n\t", it->first.c_str());
		printf("max_thread_count: %d\n\t", it->second.max_crawl_thread_count);
		printf("crawl_thread_count: %d\n\t", it->second.crawl_thread_count);
		printf("last_crawl_time: %lu\n\t", it->second.last_crawl_time);
		printf("crawl_interval: %lu\n", it->second.crawl_interval);
	}
}
