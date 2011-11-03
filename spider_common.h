#ifndef __FANFAN_SPIDER_COMMON_H__
#define __FANFAN_SPIDER_COMMON_H__

#include "sdlog.h"


#define SP_LOGNAME "spider"
#define SP_WFNAME "spiderwf"

#define ITEM_LIST "item.list"
#define CATE_LIST "cate.list"
#define FAIL_LIST "fail.list"

class CSpiderConf
{
public:
    string spider_name;                       // 该spider的名字，格式为 spiderxx
	string log_conf_path;                    // log4cxx的配置文件路径
	int spider_paused;                       // 是否暂停spider
	int next_round_continue;                 // 是否继续下一轮
	int extract_cate_url;                    // 是否提取cate
	int extract_item_url;                    // 是否提取item
	int normalize_url;

	string page_dir;                          // 网页存储的目录
	int page_name_change_interval;            // 网页文件切分的间隔

    string cate_input_path;
	string seed_path;
	string url_output_dir;
    string item_output_path;
	string cate_output_path;
	string fail_output_path;

	string stop_domain_conf_path;

	int default_max_concurrent_thread_count;  // 每个站点默认最大线程并发度
	int urlpool_empty_sleep_time;
	int max_url_len;
	int max_page_len;                         // 页面最大长度
	int min_page_len;
	
	int default_site_crawl_interval;       // 站点默认抓取间隔
	int max_url_fail_count;                   // URL抓取最大失败次数
	int max_url_redirect_count;
	int max_dns_query_count;                  // 一个URL重查IP的最大次数
	int selected_queue_empty_sleep_time;      // 抓取线程的休眠时间，单位是毫秒

	string dns_host;
	int dns_port;
	int dns_sleep_interval; //ms
	
	//crawler thread
	string extractor_conf_path;
	string recognizer_conf_path;
	
	//select thread
	int min_select_interval;
	int select_nums_per_time;
	int min_select_threshold;
	int priority_quota;
	int cate_percent;
	int item_percent;

	int selected_queue_size;

	
	//global define
	int work_thread_num;
	string statis_file_path;

	//code transfer
	string converter_code_path;
	string strong_rule_conf_path;
};

#endif


