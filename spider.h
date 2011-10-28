#ifndef __FANFAN_SPIDER_H__
#define __FANFAN_SPIDER_H__

#include <time.h>
#include "url_output.h"
#include "selected_queue.h"
#include "page_output.h"

class CPageOutput;

class CSpiderConf
{
public:
	int default_max_concurrent_thread_count;  // 每个站点默认最大线程并发度
	int urlpool_empty_sleep_time;
	int max_url_len;
	int max_page_len;                         // 页面最大长度
	
	time_t default_site_crawl_interval;       // 站点默认抓取间隔
	int max_url_fail_count;                   // URL抓取最大失败次数
	int max_dns_query_count;                  // 一个URL重查IP的最大次数
	int selected_queue_empty_sleep_time;      // 抓取线程的休眠时间，单位是毫秒

	string dns_host;
	int dns_port;
	int dns_sleep_interval; //ms
};

class CSpider
{
public:
    int load_conf(const char* conf_path);

	CSpiderConf m_spider_conf;

    CSelectedQueue* m_selected_queue;

	CUrlOutput* mp_cate_output;
	CUrlOutput* mp_item_output;
	CUrlOutput* mp_fail_output;

	CPageOutput* mp_page_output;
};

#endif
