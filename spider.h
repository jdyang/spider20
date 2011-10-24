#ifndef __FANFAN_SPIDER_H__
#define __FANFAN_SPIDER_H__

class CSpiderConf
{
	int default_max_concurrent_thread_count;  // 每个站点默认最大线程并发度
};

class CSpider
{
public:
	CSpiderConf m_spider_conf;
};

#endif
