#ifndef __FANFAN_SPIDER_H__
#define __FANFAN_SPIDER_H__

#include <time.h>
#include <string>
#include "url_output.h"
#include "selected_queue.h"
#include "page_output.h"
#include "dns_client.h"
#include "level_pool.h"

#define SP_LOGNAME "spider"
#define SP_WFNAME "spiderwf"


class CPageOutput;

class CSpiderConf
{
public:
    string spider_name;                       // 该spider的名字，格式为 spiderxx

	string page_dir;                          // 网页存储的目录
	int page_name_change_interval;            // 网页文件切分的间隔

    string item_output_path;
	string cate_output_path;
	string fail_output_path;

	int default_max_concurrent_thread_count;  // 每个站点默认最大线程并发度
	int urlpool_empty_sleep_time;
	int max_url_len;
	int max_page_len;                         // 页面最大长度
	
	int default_site_crawl_interval;       // 站点默认抓取间隔
	int max_url_fail_count;                   // URL抓取最大失败次数
	int max_url_redirect_count;
	int max_dns_query_count;                  // 一个URL重查IP的最大次数
	int selected_queue_empty_sleep_time;      // 抓取线程的休眠时间，单位是毫秒

	string dns_host;
	int dns_port;
	int dns_sleep_interval; //ms
	
	//select thread
	int min_select_interval;
};

class CSpider
{
public:
    int load_conf(const char* conf_path);
	int init(void);
    int write_page_list(CPageOutput* pout, string& url, string& domain, string& site, int flag, string& converted_content, char* page_list_buf, int page_list_buf_len);

	CSpiderConf m_spider_conf;

    CSelectedQueue* m_selected_queue;

	CUrlOutput* mp_cate_output;
	CUrlOutput* mp_item_output;
	CUrlOutput* mp_fail_output;

	CPageOutput* mp_page_output;

	CDnsClient m_dns_client;

	CLevelPool* mp_level_pool;

};

#endif
