#ifndef __FANFAN_SPIDER_H__
#define __FANFAN_SPIDER_H__

#include <time.h>

class CSpiderConf
{
public:
	int default_max_concurrent_thread_count;  // 每个站点默认最大线程并发度
	int urlpool_empty_sleep_time;
	int max_url_len;
	
	time_t default_site_crawl_interval;       // 站点默认抓取间隔
};

class CSpider
{
public:
    FuncRet load_conf(const char* conf_path);
	CSpiderConf m_spider_conf;
};

#endif
