#include "level_pool.h"
#include <time.h>

int CLevelPool::init(void)
{
	int ret = pthread_mutex_init(&m_mutex, NULL);

	return 0==ret ? 0 : -1;
}

int CLevelPool:destroy(void)
{
	int ret = pthread_mutex_destroy(&m_mutex);

	return 0==ret ? 0 : -1;
}


void CLevelPool::set_spider(CSpider* sp)
{
	mp_spider = sp;
}

void CLevelPool::insert_level_info(string& site, SLevelInfo li)
{
	m_pool.insert(make_pair(site, li));
}

bool CLevelPool::is_crawl_enabled(string& site)
{
	CSpiderConf& conf = mp_spider->m_spider_conf;

    time_t now = get_time_now();

    pthread_mutex_lock(&m_mutex);

	map<string, SLevelInfo>::iterator it = m_pool.find(site);
	if (it == m_pool.end())
	{
		SLevelInfo li;
		li.max_crawl_thread_count = conf.default_max_crawl_thread_per_site;
		li.crawl_thread_count = 1;
		last_crawl_time = now;
		li.crawl_interval = conf.site_crawl_interval;
		m_pool.insert(make_pair(site, li));

		pthread_mutex_unlock(&m_mutex);
		return true;
	}
	if (it->second.crawl_thread_count >= it.second.max_crawl_thread_count) // thread concurrent control
	{
		pthread_mutex_unlock(&m_mutex);
		return false;
	}

    if (now - it->second.last_crawl_time < it->second.crawl_interval) // site crawl interval control
	{
		pthread_mutex_unlock(&m_mutex);
		return false;
	}
	it->second.crawl_thread_count++;
	it->second.last_crawl_time = now;
	pthread_mutex_unlock(&m_mutex);
	return true;
}

void CLevelPool::finish_crawl(string& site)
{
	pthread_mutex_lock(m_mutex);
	map<string, SLevelInfo>::iterator it = m_pool.find(site);
	if (it == m_pool.end())
	{
		pthread_mutex_unlock(m_mutex);
		return;
	}
	it->second.crawl_thread_count--;
	pthread_mutex_unlock(&m_mutex);
	return;
}

time_t CLevelPool::get_time_now(void)
{
	struct timeval tm;
	gettimeofday(&tm);

	return tm.tv_sec*1000 + tm.tv_usec/1000;
}