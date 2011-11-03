#include <string>
#include <time.h>
#include <deque>
#include <iostream>
#include <algorithm>
#include <functional>
#include <vector>
#include <ctime>
#include <cstdlib>

#include "sse_common.h"
#include "sign.h"
#include "spider.h"
#include "selected_queue.h"
#include "url_output.h"
#include "page_output.h"
#include "sdconf.h"
#include "tse/Http.h"
#include "uc_url.h"
#include "base64.h"
#include "utf8_converter.h"
#include "url_recognizer.h"
#include "urlpool.h"

#define ITEM_LINK (1)
#define CATE_LINK (2)

using namespace std;

static bool stopped = false;

void* select_thread(void* arg)
{
	time_t start_time;
	time_t now_time;
	int left_time;
	
	CSpider* psp = (CSpider*)arg;
	CSpiderConf& conf = (psp->m_spider_conf);
	CSelectedQueue& sq = *(psp->mp_selected_queue);
	CDnsClient& dns_client = psp->m_dns_client;
	CLevelPool* p_level_pool = psp->mp_level_pool;
	
	while(1){
		start_time=time(NULL);
		SDLOG_INFO(SP_LOGNAME,"start updating conf.");
		if (!psp->update_conf()) {
			cerr << "load cmd conf error!" << endl;
			exit(-1);
		}
		
		SDLOG_INFO(SP_LOGNAME,"start selecting.");
		if (!psp->select_url()) {
			if (!psp->next_round()){
				SDLOG_WARN(SP_WFNAME, "next round transfer error!");
				exit(-1);
			}
			continue;
		}		//select from 4 queue
		SDLOG_INFO(SP_LOGNAME,"finish selecting.");
		SDLOG_INFO(SP_LOGNAME,"start insert urls into select queue.");
		psp->insert_url();
		SDLOG_INFO(SP_LOGNAME,"finish inserting.");
		now_time=time(NULL);
		left_time=(int)(conf.min_select_interval-difftime(now_time,start_time));
		if(left_time>0){
			usleep(left_time * 1000);
		}
	}
	
	return NULL;
}

void* crawl_thread(void* arg)
{
	CSpider* psp = (CSpider*)arg;
	CSpiderConf& conf = (psp->m_spider_conf);
	CSelectedQueue& sq = *(psp->m_selected_queue);
	CDnsClient& dns_client = psp->m_dns_client;
	CLevelPool* p_level_pool = psp->mp_level_pool;

	CUrlOutput* p_cate_output = psp->mp_cate_output;
	CUrlOutput* p_item_output = psp->mp_item_output;
	CUrlOutput* p_fail_output = psp->mp_fail_output;
	CPageOutput* p_page_output = psp->mp_page_output;

	CHttp http;
	UTF8Converter utf8_converter;
	CUrlRecognizer url_recog;

	SSQItem qi;
	
	CExtractor extractor;
	CRecognizer recognizer;
	
	if (extractor.load_conf(conf.extractor_conf_path)!=FR_OK){
		cerr << "LOAD extractor configuration failed." << endl;
		exit(1);
	}
	
	if (recognizer.load_conf(conf.recognizer_conf_path)!=FR_OK){
		cerr << "LOAD recognizer configuration failed." << endl;
		exit(1);
	}
	
	extractor.set_recognizer(&(recognizer));

	int file_length = -1;
	int redirect_count = 0;

    char* downloaded_file = NULL;
	char* file_head = NULL;
	char* location = NULL;
	int ncSock = -1;

	ExtraHttpConfig ehconfig;
	ehconfig.proxy_str = NULL;

	string redirect_url;
	string url = "";
	string site = "";
	string domain = "";
	string ip = "";

    string converted_content = "";
	string base64_content = "";

    char err_buf[4096];
	char page_list_buf[2097152];

	while (!stopped)
	{
		while (conf.spider_paused) {    // spider如果暂停
			sleep(60);
			continue;
		}
        if (!sq.pop(qi))  // SelectedQueue为空
		{
			if (-1 == p_page_output->append(NULL, 0, false)) // 为了满足即使没有抓到网页也写一个page.list空文件
			{
				printf("page append null error\n");
			}
			usleep(conf.selected_queue_empty_sleep_time*1000);
			continue;
		}

        if (qi.fail_count > conf.max_url_fail_count)  // 此URL多次抓取失败，丢弃
		{
            if (-1 == p_fail_output->append_error(qi.url, "CRAWL_FAIL"))
			{
				printf("fail append error\n");
			}
			continue;
		}
		if (qi.dns_count > conf.max_dns_query_count)  // 此URL无IP，丢弃
		{
			if (-1 == p_fail_output->append_error(qi.url, "DNS_FAIL"))
			{
				printf("fail append error\n");
			}
			continue;
		}

		ip = dns_client.get_ip(site);
		if (ip == "NO_IP")
		{
			qi.dns_count++;
			sq.push(qi);
		}

		if (!p_level_pool->is_crawl_enabled(qi.url))  // 不符合压力控制规则
		{
			sq.push(qi);
			continue;
		}

		url = qi.url;
		ucUrl uc_url(url);
		if (uc_url.build() != FR_OK)
		{
			p_level_pool->finish_crawl(site, false);
			if (-1 == p_fail_output->append_error(qi.url, "FORMAT_ERROR"))
			{
				printf("fail append error\n");
			}
			continue;
		}
		site = uc_url.get_site();
		domain = uc_url.get_domain();

        if (downloaded_file)
		{
			free(downloaded_file);
			downloaded_file = NULL;
		}
        if (file_head)
	    {
	 	    free(file_head);
			file_head = NULL;
		}
		if (location)
		{
			free(location);
			location = NULL;
		}

        ehconfig.ipstr = ip.c_str();
	    file_length = http.Fetch(url, &downloaded_file, &file_head, &location, &ncSock, &ehconfig);

		if (-404 == file_length)
		{
			printf("download 404\n");
			if (-1 == p_fail_output->append_error(url, "NOT_FOUND"))
			{
				printf("fail append error\n");
			}
			p_level_pool->finish_crawl(site);
			continue;
		}

        redirect_count = 0;
		while (-300 == file_length && redirect_count < conf.max_url_redirect_count)
		{
			if (!location)
			{
				break;
			}
			if ('/' == *location)
			{
				redirect_url = "http://" + site + location;
			}
			else
			{
				ucUrl red_url(location);
				if (red_url.build() != FR_OK)
				{
					printf("redirect url build error\n");
					break;
				}
				if (red_url.get_site() != site)
				{
					ip = dns_client.get_ip(red_url.get_site());
					if (ip == "NO_IP")
					{
						break;
					}
					ehconfig.ipstr = ip.c_str();

				}
				redirect_url = red_url.get_url();
			}

            if (downloaded_file)
			{
				free(downloaded_file);
				downloaded_file = NULL;
			}
            if (file_head)
			{
				free(file_head);
				file_head = NULL;
			}
			if (location)
			{
				free(location);
				location = NULL;
			}
			file_length = http.Fetch(redirect_url, &downloaded_file, &file_head, &location, &ncSock, &ehconfig);
			redirect_count++;
		}

		if (-404 == file_length) // 重定向到了一个404的页面
		{
			p_level_pool->finish_crawl(site);
			if (p_fail_output->append_error(url, "NOT_FOUND_REDIRECT"))
			{
				printf("fail append error");
			}
			continue;
		}

		if (file_length <= 0 && file_length > conf.max_page_len)
		{
			p_level_pool->finish_crawl(site);
			memset(err_buf, 0, sizeof(err_buf));
			sprintf(err_buf, "CONTENT_LEN_INVALID (%d)", file_length);
			if (-1 == p_fail_output->append_error(url, err_buf))
			{
				printf("fail append error\n");
			}
			continue;
		}
		p_level_pool->finish_crawl(site);

        if (qi.which_queue == QUEUE_TYPE_CPQ || qi.which_queue == QUEUE_TYPE_COQ)  // category写入cate.list
		{
			if (-1 == p_cate_output->append(qi.url))
			{
				printf("category write error\n");
				continue;
			}
		}
		else                           // item写入item.list
		{
			if (-1 == p_item_output->append(qi.url))
			{
				printf("item write error\n");
				continue;
			}
		}
		
        utf8_converter.set_input(downloaded_file, strlen(downloaded_file));
		if (-1 == utf8_converter.to_utf8())
		{
			printf("converted to utf8 error\n");
			continue;
		}
		converted_content = utf8_converter.get_converted_content();

		//extract links
		extractor.set_html(url,converted_content);
	    EcSwitch swi;
		swi.sw_link = true;
	    swi.sw_title = false;
	    swi.sw_kv = false;
	    swi.sw_img = false;
	    swi.sw_qa = false;
		swi.sw_cmt = false;
	    swi.sw_dead = false;
		swi.sw_price_img = false;
		swi.sw_light_price = false;	
		
		extractor.extract(swi);
		
		psp->write_to_queue(qi.which_queue, &extractor, &url_recog);
		SDLOG_INFO(SP_LOGNAME, "finish extracting links "<<url);

        // write item to page list
		if (qi.which_queue == QUEUE_TYPE_IPQ || qi.which_queue == QUEUE_TYPE_IOQ)
		{
		    if (-1 == psp->write_page_list(p_page_output, url, domain, site, 0, converted_content, page_list_buf, sizeof(page_list_buf)))
		    {
                printf("write page error\n");
			    continue;
		    }
		}
	}

	return NULL;
}

int CSpider::select_url()
{
	int select_nums = m_spider_conf.select_nums_per_time;
	int min_select_threshold = m_spider_conf.min_select_threshold;
	int priority_quota = m_spider_conf.priority_quota;
	int cate_percent = m_spider_conf.cate_percent/(m_spider_conf.cate_percent + m_spider_conf.item_percent);
	int select_remains = 0;
	
	vector<string> domain_p;
	vector<string> domain_o;
	map<string, vector<UrlInfo>> select_map;
	vector<UrlInfo>[m_statis.m_domain.size()] url_array;
	map<string, DomainAttr>::iterator domain_it;
	int i = -1;
	
	for (domain_it = m_statis.m_domain.begin(); domain_it != m_statis.m_domain.end(); ++domain_it) {
		if ((*domain_it).second.isSeed == 1 && (*domain_it).second.isShield == 0){
			domain_p.insert((*domain_it).first);
		} else if ((*domain_it).second.isSeed == 0 && (*domain_it).second.isShield == 0) {
			domain_o.insert((*domain_it).first);
		}
		select_map.insert(make_pair((*domain_it).first, url_array[++i]));
	}
	int o_domain_num = domain_o.size();
	int p_domain_num = domain_p.size();
	int o_link_num = m_statis.get_coq_url_num() + m_statis.get_ioq_url_num();
	int p_link_num = m_statis.get_cpq_url_num() +get_ipq_url_num();
	
	deque<UrlInfo> tmp_que = m_cpq->get_url_queue();
	deque<UrlInfo>::iterator it;
	m_cpq->get_url_mutex().lock();
	for (it = tmp_que.begin(); it != tmp_que.end(); ++it){
		(*it).type = 1;
		select_map[(*it).domain].push_back(*it);
	}
	m_cpq->get_url_queue().clear();
	m_cpq->get_url_mutex().unlock();
	
	tmp_que = m_ipq->get_url_queue();
	m_ipq->get_url_mutex().lock();
	for (it = tmp_que.begin(); it != tmp_que.end(); ++it){
		(*it).type = 0;
		select_map[(*it).domain].push_back(*it);
	}
	m_ipq->get_url_queue().clear();
	m_ipq->get_url_mutex().unlock();
	
	tmp_que = m_ioq->get_url_queue();
	m_ioq->get_url_mutex().lock();
	for (it = tmp_que.begin(); it != tmp_que.end(); ++it){
		(*it).type = 0;
		select_map[(*it).domain].push_back(*it);
	}
	m_ioq->get_url_queue().clear();
	m_ioq->get_url_mutex().unlock();
	
	tmp_que = m_icq->get_url_queue();
	m_icq->get_url_mutex().lock();
	for (it = tmp_que.begin(); it != tmp_que.end(); ++it){
		(*it).type = 1;
		select_map[(*it).domain].push_back(*it);
	}	
	m_icq->get_url_queue().clear();
	m_icq->get_url_mutex().unlock();
	
	int prio_num = select_nums*priority_quota/10;
	int ord_num = select_nums - prio_num;
	int prio_real_num = 0;
	
	vector<string>::iterator tmp_it;
	for (tmp_it = domain_p.begin(); tmp_it != domain_p.end(); ++tmp_it){
		vector<UrlInfo> tmp_vector = select_map[*tmp_it];
		int prio_num_c = prio_num * tmp_vector.size()/p_link_num*cate_percent;
		int prio_num_i = prio_num * tmp_vector.size()/p_link_num - prio_num_c + 1;
		
		int i_num = 0;
		int c_num = 0;	
		int i = 0;
		for (i = 0 ; i < tmp_vector.size() && c_num < prio_num_c && i_num < prio_num_i; ++i){
			if (tmp_vector[i].type == 1){
				++c_num;``
			} else {
				++i_num;
			}
			m_select_buffer.push_back(tmp_vector[i]);
		}
		int flag = 0;
		int tmp_num = i_num;
		int max_tmp_num = prio_num_i;
		if (c_num < prio_num_c) {
			flag = 1;
			tmp_num = c_num;
			max_tmp_num = prio_num_c;
		}
		for (int k = i; k < tmp_vector.size() && tmp_num < max_tmp_num; ++k){
			if (tmp_vector[k] == flag) {
				m_select_buffer.push_back(tmp_vector[k]);
				++tmp_num;
			} else{
				m_select_back.push_back(tmp_vector[k]);
			}
		}
		for (int l = k; l < tmp_vector.size(); ++l){
			m_select_back.push_back(tmp_vector[l]);
		}
	}
	
	//ordinay queue
	for (tmp_it = domain_o.begin(); tmp_it != domain_o.end(); ++tmp_it){
		vector<UrlInfo> tmp_vector = select_map[*tmp_it];
		int ord_num_c = ord_num * tmp_vector.size()/o_link_num*cate_percent;
		int ord_num_i = ord_num * tmp_vector.size()/o_link_num - prio_num_c + 1;
		
		int i_num = 0;
		int c_num = 0;	
		int i = 0;
		for (i = 0 ; i < tmp_vector.size() && c_num < ord_num_c && i_num < ord_num_i; ++i){
			if (tmp_vector[i].type == 1){
				++c_num;
			} else {
				++i_num;
			}
			m_select_buffer.push_back(tmp_vector[i]);
		}
		int flag = 0;
		int tmp_num = i_num;
		int max_tmp_num = ord_num_i;
		if (c_num < prio_num_c) {
			flag = 1;
			tmp_num = c_num;
			max_tmp_num = ord_num_c;
		}
		for (int k = i; k < tmp_vector.size() && tmp_num < max_tmp_num; ++k){
			if (tmp_vector[k] == flag) {
				m_select_buffer.push_back(tmp_vector[k]);
				++tmp_num;
			} else{
				m_select_back.push_back(tmp_vector[k]);
			}
		}
		for (int l = k; l < tmp_vector.size(); ++l){
			m_select_back.push_back(tmp_vector[l]);
		}
	}
	// judge if it need to go the next round
	if (m_select_rounds > 0 && m_select_buffer.size() < min_select_threshold) {
		SDLOG_INFO(SP_LOGNAME, "go to next round, count: " << m_select_rounds);
		return -1;
	}
	random_shuffle(m_select_buffer.begin(), m_select_buffer.end());

	return 0;
}

int CSpider::next_round()
{
	return 0;
}

int CSpider::insert_url()
{
	return 0;
} 

int CSpider::update_conf()
{
	return 0;
}

void CSpider::write_to_queue(int which_queue, CExtractor* extractor, CUrlRecognizer* url_recog)
{
	CSpiderConf& conf = m_spider_conf;
	UrlPool* cq;
	UrlPool* iq;

    string normal_url;
	int type;

	if (which_queue == QUEUE_TYPE_CPQ)
	{
		cq = &m_cpq;
		iq = &m_ipq;
	}
	else
	{
		cq = &m_coq;
		iq = &m_ioq;
	}

	map<string,CEcUrlLink> links = extractor->get_links();

    UrlInfo ui;
	for(map<string,CEcUrlLink>::iterator it = links.begin(); it != links.end(); ++it) 
    {
		ucUrl uc_url(it->second.link);
		if (FR_OK != uc_url.build())
		{
			printf("extracted link build error\n");
			continue;
		}
		type = url_recog->get_type(it->second.link);
		if (type == ITEM_LINK)
		{
			if (conf.extract_item_url)  // 需要提取item
			{
				if (conf.normalize_url)  // 需要归一化
				{
			        ui.url = url_recog->normalize(it->second.link);
				}
				else
				{
					ui.url = it->second.link;
				}
			    ui.last_crawl_time = 0;
			    ui.domain = uc_url.get_domain();
			    ui.site = uc_url.get_site();

                iq->push_url(ui);
			}
		}
		else if (type == CATE_LINK)
		{
			if (conf.extract_cate_url)
			{
			    ui.url = it->second.link;
			    ui.last_crawl_time = 0;
			    ui.domain = uc_url.get_domain();
			    ui.site = uc_url.get_site();
			    cq->push_url(ui);
			}
		}
		else  // 垃圾URL
		{
			printf("invalud url\n");
		}
	}
}

int CSpider::write_page_list(CPageOutput* pout, string& url, string& domain, string& site, int flag, string& converted_content, char* page_list_buf, int page_list_buf_len)
{
	string base64_content;

	unsigned int url_sign1;
	char url_sign1_buf[64];
	unsigned int url_sign2;
	char url_sign2_buf[64];
	unsigned int page_sign1;
	char page_sign1_buf[64];
	unsigned int page_sign2;
	char page_sign2_buf[64];

	char time_buf[64];
	char flag_buf[4];

    memset(time_buf, 0, sizeof(time_buf));
    memset(flag_buf, 0, sizeof(flag_buf));
    memset(url_sign1_buf, 0, sizeof(url_sign1_buf));
    memset(url_sign2_buf, 0, sizeof(url_sign2_buf));
	memset(page_sign1_buf, 0, sizeof(page_sign1_buf));
	memset(page_sign2_buf, 0, sizeof(page_sign2_buf));
    memset(page_list_buf, 0, sizeof(page_list_buf));

    sprintf(time_buf, "%lu", time(NULL));

	sprintf(flag_buf, "%d", flag);

	CSign::Generate(url.c_str(), url.length(), &url_sign1, &url_sign2);
	sprintf(url_sign1_buf, "%u", url_sign1);
	sprintf(url_sign2_buf, "%u", url_sign2);

    base64_content = base64_encode((unsigned char const*)(converted_content.c_str()), converted_content.length());
	CSign::Generate(base64_content.c_str(), base64_content.length(), &page_sign1, &page_sign2);
    sprintf(page_sign1_buf, "%u", page_sign1);
	sprintf(page_sign2_buf, "%u", page_sign2);

    int total_len = strlen(time_buf)
	              + url.length()
	              + domain.length()
				  + site.length()
				  + strlen(url_sign1_buf)
				  + strlen(url_sign2_buf)
				  + strlen(page_sign1_buf)
				  + strlen(page_sign2_buf)
				  + strlen(flag_buf)
				  + base64_content.length()
				  + 10;                               // 10 "\t"

    if (total_len >= page_list_buf_len)
	{
		return -1;
	}

    strcat(page_list_buf, time_buf);
	strcat(page_list_buf, "\t");
    sprintf(page_list_buf, url.c_str());
	sprintf(page_list_buf, "\t");
	strcat(page_list_buf, domain.c_str());
	strcat(page_list_buf, "\t");
	strcat(page_list_buf, site.c_str());
	strcat(page_list_buf, "\t");
	strcat(page_list_buf, url_sign1_buf);
	strcat(page_list_buf, "\t");
	strcat(page_list_buf, url_sign2_buf);
	strcat(page_list_buf, "\t");
	strcat(page_list_buf, page_sign1_buf);
	strcat(page_list_buf, "\t");
	strcat(page_list_buf, page_sign2_buf);
	strcat(page_list_buf, "\t");
	strcat(page_list_buf, flag_buf);
	strcat(page_list_buf, "\t");
	strcat(page_list_buf, base64_content.c_str());

    if (0 != pout->append(page_list_buf, strlen(page_list_buf)))
	{
		return -1;
	}
	return 0;
}

int CSpider::load_conf(const char* conf_path)
{
	sdConf conf;
	string str_result;
	int int_result;

	if (0 != conf.load_conf(conf_path))
	{
		printf("load conf error: %s\n", conf_path);
		return -1;
	}

    // 站点最大线程并发度
	if ((int_result=conf.get_int_item("DEFAULT_MAX_CONCURRENT_THREAD_COUNT")) <=0)
	{
		printf("get item DEFAULT_MAX_CONCURRENT_THREAD_COUNT error\n");
		return -1;
	}
	m_spider_conf.default_max_concurrent_thread_count= int_result;

    // 站点抓取间隔
	if ((int_result=conf.get_int_item("DEFAULT_SITE_CRAWL_INTERVAL")) <= 0)
	{
        printf("get item DEFAULT_SITE_CRAWL_INTERVAL error\n");
		return -1;
	}
	m_spider_conf.default_site_crawl_interval = int_result;

    // URL抓取所允许的最多失败次数
	if ((int_result=conf.get_int_item("MAX_URL_FAIL_COUNT")) <= 0)
	{
        printf("get item MAX_URL_FAIL_COUNT error\n");
		return -1;
	}
	m_spider_conf.max_url_fail_count = int_result;

    // 最大网页长度
    if ((int_result=conf.get_int_item("MAX_PAGE_LEN")) <=0)
	{
		printf("get item MAX_PAGE_LEN error\n");
		return -1;
	}
	m_spider_conf.max_page_len = int_result;

    // crawl线程发现选取队列为空时要休眠一段时间，单位是毫秒
	if ((int_result=conf.get_int_item("SELECTED_QUEUE_EMPTY_SLEEP_TIME")) <= 0)
	{
		printf("get item SELECTED_QUEUE_EMPTY_SLEEP_TIME\n");
		return -1;
	}
	m_spider_conf.selected_queue_empty_sleep_time = int_result;

	return 0;
}

int CSpider::start()
{
	//init
	if (!m_statis.init(m_spider_conf.statis_file_path) || !m_statis.write_message_to_file("start spider!")){
		cerr << "init statis file error , exit!" << endl;
		return -1;
	}
	
	if (!(mp_cate_output = new CUrlOutput()))
	{
		cerr << "malloc cate output error, exit!" << endl;
		return -1;
	}
	if (0 != mp_cate_output.init(&m_spider_conf))
	{
		cerr << "init cate output error, exit!" << endl;
		return -1;
	}

	if (!(mp_item_output = new CUrlOutput()))
	{
		cerr << "malloc item output error, exit!" << endl;
		return -1;
	}
	if (0 != mp_item_output.init(&m_spider_conf))
	{
		cerr << "init item output error, exit!" << endl;
		return -1;
	}

	if (!(mp_fail_output = new CUrlOutput()))
	{
		cerr << "malloc fail output error, exit!" << endl;
		return -1;
	}
	if (0 != mp_fail_output.init(&m_spider_conf))
	{
		cerr << "init fail output error, exit!" << endl;
		return -1;
	}

    //if (!(mp_selected_queue))

	pthread_t works[m_spider_conf.work_thread_num];
	pthread_t select;
	if (0 != pthread_create(&select, NULL, select_thread, this)) {
		cerr << "start select thread failed." << endl;
		return -1;
	}

	void* work_ret = NULL;
	for (int i = 0; i < m_spider_conf.work_thread_num; i++) {
		if (0 != pthread_create(works+i, NULL, work_thread, this)) {
			cerr << "start work thread " << i << " failed." << endl;
			pthread_join(select, &work_ret);
			for (int x=0;x<i;x++) {pthread_join(works[x],&work_ret);}
			return NULL;
		}
	}
	SDLOG_INFO(SP_LOGNAME, "spider start successfully");
	
	pthread_join(select, &work_ret);
	for (int x=0;x<m_spider_conf.work_thread_num;x++) {pthread_join(works[x],&work_ret);}
	SDLOG_INFO(SP_LOGNAME, "spider end working");
	
	return 0;
}
