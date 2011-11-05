#include <string>
#include <time.h>
#include <deque>
#include <map>
#include <iostream>
#include <algorithm>
#include <functional>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include "errno.h"

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
//	CSelectedQueue& sq = *(psp->mp_selected_queue);
//	CDnsClient& dns_client = psp->m_dns_client;
//	CLevelPool* p_level_pool = psp->mp_level_pool;
	
	if (psp->load_input_urls(conf.url_input_dir.c_str()) < 0){
		SDLOG_WARN(SP_WFNAME, "load input urls error!");
		exit(-1);
	}
	
	while(1){
		++(psp->m_select_rounds);
		start_time=time(NULL);
		SDLOG_INFO(SP_LOGNAME,"start updating conf. in the round " << psp->m_select_rounds);
		if (psp->update_conf() < 0) {
			cerr << "load cmd conf error!" << endl;
			exit(-1);
		}
		
		SDLOG_INFO(SP_LOGNAME,"start selecting.");
		if (psp->select_url() < 0) {
			SDLOG_INFO(SP_LOGNAME,"start going to next samsara.");
			if (psp->next_round() < 0){
				SDLOG_WARN(SP_WFNAME, "next samsara transfer error!");
				exit(-1);
			}
			if (psp->load_input_urls(conf.url_output_dir.c_str()) < 0){
				SDLOG_WARN(SP_WFNAME, "load input urls error!");
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
		SDLOG_INFO(SP_LOGNAME,"start going to next samsara.");
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
	CSelectedQueue& sq = *(psp->mp_selected_queue);
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
	
	if (0 != utf8_converter.init(conf.converter_code_path.c_str()))
	{
		cerr << "utf8 init error: " << conf.converter_code_path << endl;
		exit(1);
	}

	if (0 != url_recog.load_conf(conf.strong_rule_conf_path.c_str()))
	{
		cerr << "strong rules load conf error" << endl;
		exit(1);
	}

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

        if (!psp->mp_selected_queue->pop(qi))  // SelectedQueue为空
		{
			if (-1 == p_page_output->append(NULL, 0, false)) // 为了满足即使没有抓到网页也写一个page.list空文件
			{
				SDLOG_WARN(SP_WFNAME, "write null page error");
			}
			usleep(conf.selected_queue_empty_sleep_time*1000);
			continue;
		}

        if (qi.fail_count > conf.max_url_fail_count)  // 此URL多次抓取失败，丢弃
		{
			SDLOG_INFO(SP_LOGNAME, "CRAWL_FAIL\t"<<qi.url)
            if (-1 == p_fail_output->append_error(qi.url, "CRAWL_FAIL"))
			{
				SDLOG_WARN(SP_WFNAME, "fail append error\t"<<qi.url);
			}
			continue;
		}
		if (qi.dns_count > conf.max_dns_query_count)  // 此URL无IP，丢弃
		{
			SDLOG_INFO(SP_LOGNAME, "DNS_FAIL\t"<<qi.url)
			if (-1 == p_fail_output->append_error(qi.url, "DNS_FAIL"))
			{
				SDLOG_WARN(SP_WFNAME, "fail append error\t"<<qi.url);
			}
			continue;
		}

		ip = dns_client.get_ip(site);
		if (ip == "NO_IP")
		{
			SDLOG_WARN(SP_WFNAME, "get ip fail\t"<<qi.url);
			qi.dns_count++;
			sq.push(qi);
		}

		if (!p_level_pool->is_crawl_enabled(qi.url))  // 不符合压力控制规则
		{
			//SDLOG_INFO(SP_LOGNAME, "DELAY\t"<<qi.url);
			psp->mp_selected_queue->push(qi);
			continue;
		}

		url = qi.url;
		ucUrl uc_url(url);
		if (uc_url.build() != FR_OK)
		{
			p_level_pool->finish_crawl(site, false);
			if (-1 == p_fail_output->append_error(qi.url, "FORMAT_ERROR"))
			{
				SDLOG_INFO(SP_LOGNAME, "fail append error\t"<<url);
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
			SDLOG_INFO(SP_LOGNAME, "NOT_FOUND\t"<<url);
			if (-1 == p_fail_output->append_error(url, "NOT_FOUND"))
			{
				SDLOG_WARN(SP_WFNAME, "fail append error\t"<<url);
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
					SDLOG_INFO(SP_LOGNAME, "REDIRECT_BUILD_FAIL\t"<< url << " -> " <<redirect_url);
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
			SDLOG_INFO(SP_LOGNAME, "REDIRECT_NOT_FOUND\t"<<url);
			if (p_fail_output->append_error(url, "REDIRECT_NOT_FOUND"))
			{
				SDLOG_WARN(SP_WFNAME, "fail append error\t"<<url);
			}
			continue;
		}

        if (file_length < 0) {
			p_level_pool->finish_crawl(site);
			SDLOG_INFO(SP_LOGNAME, "NET_FAIL\t"<<url);
			qi.fail_count++;
			psp->mp_selected_queue->push(qi);
			continue;
		}
		if (file_length <= conf.min_page_len && file_length > conf.max_page_len)
		{
			p_level_pool->finish_crawl(site);
			memset(err_buf, 0, sizeof(err_buf));
			SDLOG_INFO(SP_LOGNAME, "SIZE_FAIL\t"<<url);
			sprintf(err_buf, "CONTENT_LEN_INVALID (%d)", file_length);
			if (-1 == p_fail_output->append_error(url, err_buf))
			{
				SDLOG_WARN(SP_WFNAME, "fail append error\t"<<url);
			}
			continue;
		}
		p_level_pool->finish_crawl(site);
		SDLOG_INFO(SP_LOGNAME, "SUCCESS\t"<<url);

        if (qi.which_queue == QUEUE_TYPE_CPQ || qi.which_queue == QUEUE_TYPE_COQ)  // category写入cate.list
		{
			if (-1 == p_cate_output->append(qi.url))
			{
				SDLOG_WARN(SP_WFNAME, "category append error\t"<<url);
				continue;
			}
		}
		else                           // item写入item.list
		{
			if (-1 == p_item_output->append(qi.url))
			{
				SDLOG_WARN(SP_WFNAME, "item append error\t"<<url);
				continue;
			}
		}
		
        utf8_converter.set_input(downloaded_file, strlen(downloaded_file));
		if (-1 == utf8_converter.to_utf8())
		{
			SDLOG_WARN(SP_WFNAME, "converted to utf8 error\t"<<url);
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
				SDLOG_WARN(SP_WFNAME, "page append error\t"<<url);
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
	float cate_percent = 1.0*m_spider_conf.cate_percent/(m_spider_conf.cate_percent + m_spider_conf.item_percent);
	
//	int select_remains = 0;
	
	vector<string> domain_p;
	vector<string> domain_o;
	map<string, vector<UrlInfo> > select_map;
	vector<vector<UrlInfo> > url_array;
	map<string, DomainAttr>::iterator domain_it;
	int i = -1;
	
	for (domain_it = m_statis.m_domain.begin(); domain_it != m_statis.m_domain.end(); ++domain_it) {
		if ((*domain_it).second.isSeed == 1 && (*domain_it).second.isShield == 0){
			domain_p.push_back((*domain_it).first);
		} else if ((*domain_it).second.isSeed == 0 && (*domain_it).second.isShield == 0) {
			domain_o.push_back((*domain_it).first);
		}
		SDLOG_INFO(SP_LOGNAME, "add domain " << (*domain_it).first);
		vector<UrlInfo> tmp;
		url_array.push_back(tmp);
		select_map.insert(make_pair((*domain_it).first, url_array[++i]));
	}
//	int o_domain_num = domain_o.size();
//	int p_domain_num = domain_p.size();
//	int o_link_num = m_statis.get_coq_url_num() + m_statis.get_ioq_url_num();
//	int p_link_num = m_statis.get_cpq_url_num() + m_statis.get_ipq_url_num();
	
	int o_link_num = mp_coq->get_url_queue().size() + mp_ioq->get_url_queue().size();
	int p_link_num = mp_cpq->get_url_queue().size() + mp_ipq->get_url_queue().size();
	
	deque<UrlInfo> tmp_que = mp_cpq->get_url_queue();
	deque<UrlInfo>::iterator it;
	mp_ipq->get_url_mutex().lock();
	for (it = tmp_que.begin(); it != tmp_que.end(); ++it){
		(*it).type = 3;
		select_map[(*it).domain].push_back(*it);
	}
	mp_cpq->get_url_queue().clear();
	mp_cpq->get_url_mutex().unlock();
	
	tmp_que = mp_ipq->get_url_queue();
	mp_ipq->get_url_mutex().lock();
	for (it = tmp_que.begin(); it != tmp_que.end(); ++it){
		(*it).type = 2;
		select_map[(*it).domain].push_back(*it);
	}
	mp_ipq->get_url_queue().clear();
	mp_ipq->get_url_mutex().unlock();
	
	tmp_que = mp_coq->get_url_queue();
	mp_ioq->get_url_mutex().lock();
	for (it = tmp_que.begin(); it != tmp_que.end(); ++it){
		(*it).type = 1;
		select_map[(*it).domain].push_back(*it);
	}
	mp_ioq->get_url_queue().clear();
	mp_ioq->get_url_mutex().unlock();
	
	tmp_que = mp_coq->get_url_queue();
	mp_coq->get_url_mutex().lock();
	for (it = tmp_que.begin(); it != tmp_que.end(); ++it){
		(*it).type = 1;
		select_map[(*it).domain].push_back(*it);
	}	
	mp_coq->get_url_queue().clear();
	mp_coq->get_url_mutex().unlock();
	
	int prio_num = select_nums*priority_quota/10;
	int ord_num = select_nums - prio_num;
	int prio_count = 0;
//	int prio_real_num = 0;
	
	vector<string>::iterator tmp_it;
	for (tmp_it = domain_p.begin(); tmp_it != domain_p.end(); ++tmp_it){
		vector<UrlInfo> tmp_vector = select_map[*tmp_it];
		int prio_num_c = (cate_percent * prio_num) * tmp_vector.size()/p_link_num;
		int prio_num_i = prio_num * tmp_vector.size()/p_link_num - prio_num_c + 1;
		if (prio_num_c < 10)prio_num_c = 10;
		if (prio_num_i < 10)prio_num_i = 10;
		
		SDLOG_INFO(SP_LOGNAME, "priority domain " << *tmp_it << " selected cate: " << prio_num_c << " and item : " << prio_num_i);
		
		int i_num = 0;
		int c_num = 0;	
		unsigned int i = 0;
		for (i = 0 ; i < tmp_vector.size() && c_num < prio_num_c && i_num < prio_num_i; ++i){
			if (tmp_vector[i].type == 3){
				++c_num;
			} else {
				++i_num;
			}
			m_sites.insert(make_pair(tmp_vector[i].site, "NO_IP"));
			m_select_buffer.push_back(tmp_vector[i]);
			++prio_count;
		}
		int flag = 2;
		int tmp_num = i_num;
		int max_tmp_num = prio_num_i;
		if (c_num < prio_num_c) {
			flag = 3;
			tmp_num = c_num;
			max_tmp_num = prio_num_c;
		}
		//the rest of c or i
		unsigned int k = i;
		for (;k < tmp_vector.size() && tmp_num < max_tmp_num; ++k){
			if (tmp_vector[k].type == flag) {
				m_sites.insert(make_pair(tmp_vector[k].site, "NO_IP"));
				m_select_buffer.push_back(tmp_vector[k]);
				++prio_count;
				++tmp_num;
			} else{
				m_select_back_p.push_back(tmp_vector[k]);
				++prio_count;
			}
		}
		for (unsigned int l = k; l < tmp_vector.size(); ++l){
			m_select_back_p.push_back(tmp_vector[l]);
			++prio_count;
		}
	}
	
	if (prio_count < prio_num){
		ord_num = ord_num + prio_num -prio_count;
	}
	
	//ordinay queue
	for (tmp_it = domain_o.begin(); tmp_it != domain_o.end(); ++tmp_it){
		vector<UrlInfo> tmp_vector = select_map[*tmp_it];
		int ord_num_c = (cate_percent * ord_num) * tmp_vector.size()/o_link_num;
		int ord_num_i = ord_num * tmp_vector.size()/o_link_num - ord_num_c + 1;
		if (ord_num_c < 10)ord_num_c = 10;
		if (ord_num_i < 10)ord_num_i = 10;
		
		SDLOG_INFO(SP_LOGNAME, "ordinary domain " << *tmp_it << " selected cate: " << ord_num_c << " and item : " << ord_num_i);
		
		int i_num = 0;
		int c_num = 0;	
		unsigned int i = 0;
		for (i = 0 ; i < tmp_vector.size() && c_num < ord_num_c && i_num < ord_num_i; ++i){
			if (tmp_vector[i].type == 1){
				++c_num;
			} else {
				++i_num;
			}
			m_sites.insert(make_pair(tmp_vector[i].site, "NO_IP"));
			m_select_buffer.push_back(tmp_vector[i]);
		}
		int flag = 0;
		int tmp_num = i_num;
		int max_tmp_num = ord_num_i;
		if (c_num < ord_num_c) {
			flag = 1;
			tmp_num = c_num;
			max_tmp_num = ord_num_c;
		}
		unsigned int k = i;
		for (; k < tmp_vector.size() && tmp_num < max_tmp_num; ++k){
			if (tmp_vector[k].type == flag) {
				m_sites.insert(make_pair(tmp_vector[k].site, "NO_IP"));
				m_select_buffer.push_back(tmp_vector[k]);
				++tmp_num;
			} else{
				m_select_back_o.push_back(tmp_vector[k]);
			}
		}
		for (unsigned int l = k; l < tmp_vector.size(); ++l){
			m_select_back_o.push_back(tmp_vector[l]);
		}
	}
	// judge if it need to go the next round
	if (m_select_rounds > 0 && m_select_buffer.size() < (unsigned int)min_select_threshold) {
		SDLOG_INFO(SP_LOGNAME, "go to next round, count: " << m_select_rounds);
		return -1;
	}
	random_shuffle(m_select_buffer.begin(), m_select_buffer.end());

	return 0;
}

int CSpider::next_round()
{
	string url_path = m_spider_conf.url_output_dir;
	m_statis.write_message_to_file("next samsara");
	
	mp_cate_output->destroy();
	mp_item_output->destroy();
	mp_fail_output->destroy();
	if (transfer() < 0){
		SDLOG_WARN(SP_WFNAME,"transfer: errror");
		return -1;
	}
	string path = m_spider_conf.url_output_dir + "/" + INPUT_PATH;
	mp_cate_output->init((path+"/"+ITEM_LIST).c_str());
	mp_item_output->init((path+"/"+CATE_LIST).c_str());
	mp_fail_output->init((path+"/"+FAIL_LIST).c_str());
	
	return 0;
}

int CSpider::transfer()
{
	string str_saver_path =  m_spider_conf.url_output_dir + "/" + OUTPUT_PATH;
	string str_reader_path =  m_spider_conf.url_output_dir + "/" + INPUT_PATH;
	string str_backup_path =  m_spider_conf.url_output_dir + "/" + BACKUP_PATH;
	/*
	step1: mv reader backup
	step2: mv saver reader
	step2: mkdir saver
	*/
	
	/*
	 * while sytem is executed, parent process shoud wait for chile process
	*/
	signal(SIGCHLD,SIG_DFL);

	SDLOG_INFO(SP_LOGNAME,"start transfer rm b.");
	string str_cmd = "rm -rf " +  str_backup_path;
	if (system(str_cmd.c_str()) ==-1){
		SDLOG_WARN(SP_WFNAME,"transfer: failed to execute command " << str_cmd);
		SDLOG_WARN(SP_WFNAME,"transfer: errror " << strerror(errno));
	}

	SDLOG_INFO(SP_LOGNAME,"start transfer r2b.");
	str_cmd = "mv -f " + str_reader_path + " " + str_backup_path;
	if (system(str_cmd.c_str()) ==-1){
		SDLOG_WARN(SP_WFNAME,"transfer: failed to execute command " << str_cmd.c_str());
		SDLOG_WARN(SP_WFNAME,"transfer: errror " << strerror(errno));
		return -1;
	}
	SDLOG_INFO(SP_LOGNAME,"start transfer s2r.");

	str_cmd = "mv -f " + str_saver_path + " " + str_reader_path;
	if (system(str_cmd.c_str()) ==-1){
		SDLOG_WARN(SP_WFNAME,"transfer: failed to execute command " << str_cmd.c_str());
		SDLOG_WARN(SP_WFNAME,"transfer: errror " << strerror(errno));
		return -1;
	}
	SDLOG_INFO(SP_LOGNAME,"start make s.");	
	str_cmd = "mkdir -p " + str_saver_path;
	if (system(str_cmd.c_str()) ==-1){
		SDLOG_WARN(SP_WFNAME,"transfer: failed to execute command << " << str_cmd.c_str());
		SDLOG_WARN(SP_WFNAME,"transfer: errror " << strerror(errno));
		return -1;
	}
	signal(SIGCHLD,SIG_IGN); 

	SDLOG_INFO(SP_LOGNAME,"finish transfer");	
	
	return true;
}

int CSpider::insert_url()
{
	//push back to urlpool
	vector<UrlInfo>::iterator it;
	for (it = m_select_back_o.begin(); it != m_select_back_o.end(); ++it){
		if ((*it).type == 1){
			mp_coq->add_to_que(*it);
		}else if ((*it).type == 0){
			mp_ioq->add_to_que(*it);
		}
	}
	for (it = m_select_back_p.begin(); it != m_select_back_p.end(); ++it){
		if ((*it).type == 3){
			mp_cpq->add_to_que(*it);
		}else if ((*it).type == 2){
			mp_ipq->add_to_que(*it);
		}
	}
	//get the ips ready
	m_dns_client.clear_map();
	map<string, string>::iterator site_it;
	for (site_it = m_sites.begin(); site_it != m_sites.end(); ++site_it){
		string ip = m_dns_client.get_ip((*site_it).first);
		(*site_it).second = ip;
	}
	
	//push into sq
	for (it = m_select_buffer.begin(); it != m_select_buffer.end(); ++it) {
		SSQItem item;
		item.url = (*it).url;
		item.fail_count = 0;
		item.dns_count = 0;
		item.last_crawl_time = (*it).last_crawl_time;
		switch((*it).type) {
			case 0:item.which_queue = QUEUE_TYPE_IOQ;break;
			case 1:item.which_queue = QUEUE_TYPE_COQ;break;
			case 2:item.which_queue = QUEUE_TYPE_IPQ;break;
			case 3:item.which_queue = QUEUE_TYPE_CPQ;break;
			default:item.which_queue = QUEUE_TYPE_COQ;break;

		}
		mp_selected_queue->push(item);
	}
	
	return 0;
} 

long CSpider::get_change_time(const char* path)
{
	struct stat file_stat;
	if (-1 == stat(path, &file_stat))
	{
		return -1;
	}
	return file_stat.st_ctime;
}

int CSpider::update_conf()
{
	CSpiderConf& conf = m_spider_conf;
	long change_time;

    // 更新主配置文件
	if (-1 == (change_time=get_change_time(m_conf_path.c_str())))
	{
		SDLOG_WARN(SP_WFNAME, "detect conf change error");
		return -1;
	}
	if (change_time != m_conf_change_time)
	{
		if (-1 == load_conf(m_conf_path.c_str()))
		{
			SDLOG_WARN(SP_WFNAME, "reload conf error");
			return -1;
		}
		m_conf_change_time = change_time;
	}

    // 更新要屏蔽的domain
	if (-1 == (change_time=get_change_time(conf.stop_domain_conf_path.c_str())))
	{
		SDLOG_WARN(SP_WFNAME, "detect stop domain conf change error");
		return -1;
	}
	if (change_time != m_stop_domain_conf_change_time)
	{
		if (-1 == load_stop_domain(conf.stop_domain_conf_path.c_str()))
		{
			SDLOG_WARN(SP_WFNAME, "load stop domain error");
			return -1;
		}
		m_stop_domain_conf_change_time = change_time;
	}

    // 新加种子
	if (-1 == access(conf.seed_path.c_str(), 0))
	{
		SDLOG_INFO(SP_LOGNAME, "seed file [" << conf.seed_path << "] not exist");
		return 0;
	}
	if (-1 == (change_time=get_change_time(conf.seed_path.c_str())))
	{
		SDLOG_WARN(SP_WFNAME, "detect seed change error");
		return -1;
	}
	if (change_time != m_seed_change_time)
	{
		if (-1 == load_seed(conf.seed_path.c_str()))
		{
			SDLOG_WARN(SP_WFNAME, "load seed error");
			return -1;
		}
		m_seed_change_time = change_time;
	}

	return 0;
}

int CSpider::load_stop_domain(const char* stop_file)
{
	CSpiderConf& conf = m_spider_conf;

	FILE* fp = NULL;
	char line[conf.max_url_len+1];
	string domain = "";
	int lineno = 0;
	char* p = NULL;
	DomainAttr da;

	fp = fopen(stop_file, "r");
	if (!fp)
	{
		SDLOG_WARN(SP_WFNAME, "open stop domain file error "<<stop_file);
		return -1;
	}

    // 清空原有的配置
	map<string, DomainAttr>::iterator it = m_statis.m_domain.begin();
	for (; it != m_statis.m_domain.end(); it++)
	{
		it->second.isShield = false;
	}

    int count = 0;
    while (!feof(fp))
	{
		memset(line, 0, sizeof(line));
		get_one_line(fp, line, sizeof(line), lineno);
		p = filter_headtail_blank(line);
		if (p[0] == '\0')
		{
			SDLOG_INFO(SP_LOGNAME, "load stop domain: find an empty domain");
			continue;
		}
		count++;

		domain = p;
		map<string, DomainAttr>::iterator dit = m_statis.m_domain.find(domain);
		if (dit == m_statis.m_domain.end())
		{
			da.isShield = true;
			da.isSeed = false;
			m_statis.m_domain.insert(make_pair(domain, da));
			SDLOG_INFO(SP_LOGNAME, "stop select "<< domain);
		}
		else
		{
		    dit->second.isShield = true;
			SDLOG_INFO(SP_LOGNAME, "stop select "<< domain);
		}
	}
	SDLOG_INFO(SP_LOGNAME, "stop " << count << " domains");

	return 0;
}

int CSpider::load_input_urls(const char* input_path)
{
	CSpiderConf& conf = m_spider_conf;
	
	string cate_path = string(input_path) + "/" + CATE_LIST;
	string item_path = string(input_path) + "/" + ITEM_LIST;
	
	m_statis.m_domain.clear();

	FILE* fp = NULL;
	char line[conf.max_url_len+1];
	UrlInfo ui;
	int lineno = 0;
	char* p = NULL;

	int count = 0;

	fp = fopen(cate_path.c_str(), "r");
	if (!fp)
	{
		SDLOG_WARN(SP_WFNAME, "open cate file error: "<<cate_path);
		return -1;
	}

	while (!feof(fp))
	{
		memset(line, 0, sizeof(line));
		get_one_line(fp, line, sizeof(line), lineno);
		p = filter_headtail_blank(line);
		if (p[0] == '\0')
		{
			SDLOG_INFO(SP_LOGNAME, "find an empty seed");
			continue;
		}
		for (unsigned int i = 0; i< sizeof(line); ++i){
			if (p[i] == '\t'){
				p[i] = '\0';
				break;
			}
		}
		ucUrl uc_url(p);
		if (uc_url.build() != FR_OK)
		{
			SDLOG_INFO(SP_LOGNAME, "seed build error for\t" << p);
			continue;
		}

        count++;

		ui.url = uc_url.get_url();
		ui.site = uc_url.get_site();
		ui.domain = uc_url.get_domain();
		ui.last_crawl_time = 0;
		mp_coq->push_url(ui);

        map<string, DomainAttr>::iterator it = m_statis.m_domain.find(ui.domain);
		if (it == m_statis.m_domain.end())
		{
            DomainAttr da;
			da.isShield = false;
			da.isSeed = false;
			m_statis.m_domain.insert(make_pair(ui.domain, da));
		}
	}
	fclose(fp);
	int real_num = m_statis.get_coq_url_num() + count;
	m_statis.set_coq_url_num(real_num);
	SDLOG_INFO(SP_LOGNAME, "load " << count << " cate success");
	
	count = 0;
	fp = fopen(item_path.c_str(), "r");
	if (!fp)
	{
		SDLOG_WARN(SP_WFNAME, "open item file error: "<<cate_path);
		return -1;
	}

	while (!feof(fp))
	{
		memset(line, 0, sizeof(line));
		get_one_line(fp, line, sizeof(line), lineno);
		p = filter_headtail_blank(line);
		if (p[0] == '\0')
		{
			SDLOG_INFO(SP_LOGNAME, "find an empty seed");
			continue;
		}
		for (unsigned int i = 0; i< sizeof(line); ++i){
			if (p[i] == '\t'){
				p[i] = '\0';
				break;
			}
		}		
		ucUrl uc_url(p);
		if (uc_url.build() != FR_OK)
		{
			SDLOG_INFO(SP_LOGNAME, "seed build error for\t" << p);
			continue;
		}

        count++;

		ui.url = uc_url.get_url();
		ui.site = uc_url.get_site();
		ui.domain = uc_url.get_domain();
		ui.last_crawl_time = 0;
		mp_ioq->push_url(ui);

        map<string, DomainAttr>::iterator it = m_statis.m_domain.find(ui.domain);
		if (it == m_statis.m_domain.end())
		{
            DomainAttr da;
			da.isShield = false;
			da.isSeed = false;
			m_statis.m_domain.insert(make_pair(ui.domain, da));
		}
	}
	fclose(fp);
	real_num = m_statis.get_ioq_url_num() + count;
	m_statis.set_ioq_url_num(real_num);
	SDLOG_INFO(SP_LOGNAME, "load " << count << " item success");

	return 0;
}

int CSpider::load_seed(const char* seed_path)
{
	CSpiderConf& conf = m_spider_conf;

	FILE* fp = NULL;
	FILE* bfp = NULL;

	char line[conf.max_url_len+1];
	UrlInfo ui;
	int lineno = 0;
	char* p = NULL;

	int count = 0;

	fp = fopen(seed_path, "r");  // 新增种子
	if (!fp)
	{
		SDLOG_WARN(SP_WFNAME, "open seed file error: "<<seed_path);
		return -1;
	}

	bfp = fopen(conf.seed_bak_path.c_str(), "a");
	if (!bfp)
	{
		SDLOG_WARN(SP_WFNAME, "open seed bak file error: " << conf.seed_bak_path);
		return -1;
	}

	fprintf(fp, "\n"); // 每次增加种子以空行分割

	while (!feof(fp))
	{
		memset(line, 0, sizeof(line));
		get_one_line(fp, line, sizeof(line), lineno);
		p = filter_headtail_blank(line);
		if (p[0] == '\0')
		{
			SDLOG_INFO(SP_LOGNAME, "find an empty seed");
			continue;
		}

		ucUrl uc_url(p);
		if (uc_url.build() != FR_OK)
		{
			SDLOG_INFO(SP_LOGNAME, "seed build error for\t" << p);
			continue;
		}

        count++;
		fprintf(fp, "%s\n", p); // 追加到种子备份文件中

		ui.url = uc_url.get_url();
		ui.site = uc_url.get_site();
		ui.domain = uc_url.get_domain();
		ui.last_crawl_time = 0;
		mp_cpq->push_url(ui);

        map<string, DomainAttr>::iterator it = m_statis.m_domain.find(ui.domain);
		if (it == m_statis.m_domain.end())
		{
            DomainAttr da;
			da.isShield = false;
			da.isSeed = true;
			m_statis.m_domain.insert(make_pair(ui.domain, da));
		}
		else if (!(it->second.isSeed))
		{
			it->second.isSeed = true;
		}
	}
	SDLOG_INFO(SP_LOGNAME, "load " << count << " seeds success");
	fclose(fp);
	fclose(bfp);

	return 0;
}

void CSpider::get_one_line(FILE* fp, char* line, int len, int& lineno)
{
    lineno++;
    memset(line,0,len);
    if(feof(fp)) return;

    fgets(line,len,fp);
    char* pch = strchr(line,'\r');
    if(pch != NULL) *pch = 0;
    pch = strchr(line,'\n');
    if(pch != NULL) *pch = 0;
}

char* CSpider::filter_headtail_blank(char* buf, int len)
{
    char *p = buf;
    char *q = buf + len -1;
    while (true)
    {
        if (*p != ' ' && *p != '\t' && *p != '\n') break;
        *p++ = '\0';
    }

    while (true)
    {
        if (*q != ' ' && *q != '\t' && *q != '\n' && *q != '\0') break;
        *q-- = '\0';
    }
    return p;
}

void CSpider::write_to_queue(int which_queue, CExtractor* extractor, CUrlRecognizer* url_recog)
{
	CSpiderConf& conf = m_spider_conf;
	CUrlPool* cq;
	CUrlPool* iq;

    string normal_url;
	int type;

	if (which_queue == QUEUE_TYPE_CPQ)
	{
		cq = mp_cpq;
		iq = mp_ipq;
	}
	else
	{
		cq = mp_coq;
		iq = mp_ioq;
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
			SDLOG_INFO(SP_LOGNAME, "invalud url\t"<<it->second.link);
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
		SDLOG_INFO(SP_LOGNAME, "page too len:\t" << url);
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
		SDLOG_INFO(SP_LOGNAME, "write page error:\t"<<url);
		return -1;
	}
	return 0;
}

int CSpider::load_conf(const char* conf_path)
{
	sdConf conf;
	string str_result;
	int int_result;

    m_conf_path = conf_path;

	if (FR_OK != conf.load_conf(conf_path))
	{
		printf("load conf error: %s\n", conf_path);
		return -1;
	}

    // 该spider服务的名字
	if ((str_result=conf.get_string_item("SPIDER_NAME")).empty())
	{
		printf("get item SPIDER_NAME error\n");
		return -1;
	}
	m_spider_conf.spider_name = str_result;
    // Log4cxx配置文件路径
	if ((str_result=conf.get_string_item("LOG_CONF_PATH")).empty())
	{
		printf("get item LOG_CONF_PATH error\n");
		return -1;
	}
	m_spider_conf.log_conf_path = str_result;
	// 是否暂停spider
	if ((int_result=conf.get_int_item("SPIDER_PAUSED")) < 0)
	{
		printf("get item SPIDER_PAUSED error\n");
		return -1;
	}
	m_spider_conf.spider_paused = int_result;
	// 是否继续下一轮
	if ((int_result=conf.get_int_item("NEXT_ROUND_CONTINUE")) < 0)
	{
		printf("get item NEXT_ROUND_CONTINUE error\n");
		return -1;
	}
	m_spider_conf.next_round_continue = int_result;
	// 是否提取category
	if ((int_result=conf.get_int_item("EXTRACT_CATE_URL"))<0)
	{
		printf("get item EXTRACT_CATE_URL error\n");
		return -1;
	}
	m_spider_conf.extract_cate_url = int_result;
	// 是否提取item
	if ((int_result=conf.get_int_item("EXTRACT_ITEM_URL"))<0)
	{
		printf("get item EXTRACT_ITEM_URL error\n");
		return -1;
	}
	m_spider_conf.extract_item_url = int_result;
	// item是否要归一化
	if ((int_result=conf.get_int_item("NORMALIZE_URL"))<0)
	{
		printf("get item NORMALIZE_URL error\n");
		return -1;
	}
	m_spider_conf.normalize_url = int_result;
    // 网页文件存储的目录
	if ((str_result=conf.get_string_item("PAGE_DIR")).empty())
	{
		printf("get item PAGE_DIR error\n");
		return -1;
	}
	m_spider_conf.page_dir = str_result;
	// 网页文件每隔多少分钟输出一个新文件
	if ((int_result=conf.get_int_item("PAGE_NAME_CHANGE_INTERVAL")) < 0)
	{
		printf("get item PAGE_NAME_CHANGE_INTERVAL error\n");
		return -1;
	}
	m_spider_conf.page_name_change_interval = int_result;
	//category输入路径
	if ((str_result=conf.get_string_item("CATE_INPUT_PATH")).empty())
	{
		printf("get item CATE_INPUT_PATH error\n");
		return -1;
	}
	m_spider_conf.cate_input_path = str_result;
	//种子路径
	if ((str_result=conf.get_string_item("SEED_PATH")).empty())
	{
		printf("get item SEED_PATH error\n");
		return -1;
	}
	m_spider_conf.seed_path = str_result;
	//种子备份路径
	if ((str_result=conf.get_string_item("SEED_BAK_PATH")).empty())
	{
		printf("get item SEED_BAK_PATH error\n");
		return -1;
	}
	m_spider_conf.seed_bak_path = str_result;
	// URL输出路径
	if ((str_result=conf.get_string_item("URL_OUTPUT_DIR")).empty())
	{
		printf("get item URL_OUTPUT_DIR error\n");
		return -1;
	}
	m_spider_conf.url_output_dir = str_result;
	if ((str_result=conf.get_string_item("URL_INPUT_DIR")).empty())
	{
		printf("get item URL_INPUT_DIR error\n");
		return -1;
	}
	m_spider_conf.url_input_dir = str_result;
	// item连接的输出路径
	if ((str_result=conf.get_string_item("ITEM_OUTPUT_PATH")).empty())
	{
		printf("get item ITEM_OUTPUT_PATH error\n");
		return -1;
	}
	m_spider_conf.item_output_path = str_result;
	// category连接的输出路径
	if ((str_result=conf.get_string_item("CATE_OUTPUT_PATH")).empty())
	{
		printf("get item CATE_OUTPUT_PATH error\n");
		return -1;
	}
	m_spider_conf.cate_output_path = str_result;
	// fail连接的输出路径
	if ((str_result=conf.get_string_item("FAIL_OUTPUT_PATH")).empty())
	{
		printf("get item FAIL_OUTPUT_PATH error\n");
		return -1;
	}
	m_spider_conf.fail_output_path = str_result;

	// 屏蔽配置文件路径
	if ((str_result=conf.get_string_item("STOP_DOMAIN_CONF_PATH")).empty())
	{
		printf("get item STOP_DOMAIN_CONF_PATH error\n");
		return -1;
	}
	m_spider_conf.stop_domain_conf_path = str_result;


    // 站点最大线程并发度
	if ((int_result=conf.get_int_item("DEFAULT_MAX_CONCURRENT_THREAD_COUNT")) <=0)
	{
		printf("get item DEFAULT_MAX_CONCURRENT_THREAD_COUNT error\n");
		return -1;
	}
	m_spider_conf.default_max_concurrent_thread_count= int_result;

    // urlpool为空
	if ((int_result=conf.get_int_item("URLPOOL_EMPTY_SLEEP_TIME")) <= 0)
	{
		printf("get item URLPOOL_EMPTY_SLEEP_TIME\n");
		return -1;
	}
	m_spider_conf.urlpool_empty_sleep_time = int_result;
    // 最大URL长度
    if ((int_result=conf.get_int_item("MAX_URL_LEN")) <= 0)
	{
		printf("get item MAX_URL_LEN error\n");
		return -1;
	}
	m_spider_conf.max_url_len = int_result;
    // 最大网页长度
    if ((int_result=conf.get_int_item("MAX_PAGE_LEN")) <=0)
	{
		printf("get item MAX_PAGE_LEN error\n");
		return -1;
	}
	m_spider_conf.max_page_len = int_result;
    // 最小网页长度
    if ((int_result=conf.get_int_item("MIN_PAGE_LEN")) <=0)
	{
		printf("get item MIN_PAGE_LEN error\n");
		return -1;
	}
	m_spider_conf.min_page_len = int_result;
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
    // URL最多重复抓取次数
	if ((int_result=conf.get_int_item("MAX_URL_REDIRECT_COUNT")) <= 0)
	{
        printf("get item MAX_URL_REDIRECT_COUNT error\n");
		return -1;
	}
	m_spider_conf.max_url_redirect_count = int_result;
    // 最多重复查询DNS的次数
	if ((int_result=conf.get_int_item("MAX_DNS_QUERY_COUNT")) <= 0)
	{
        printf("get item MAX_DNS_QUERY_COUNT error\n");
		return -1;
	}
	m_spider_conf.max_dns_query_count = int_result;
    // crawl线程发现选取队列为空时要休眠一段时间，单位是毫秒
	if ((int_result=conf.get_int_item("SELECTED_QUEUE_EMPTY_SLEEP_TIME")) <= 0)
	{
		printf("get item SELECTED_QUEUE_EMPTY_SLEEP_TIME error\n");
		return -1;
	}
	m_spider_conf.selected_queue_empty_sleep_time = int_result;
    // DNS服务器地址
	if ((str_result=conf.get_int_item("DNS_HOST")).empty())
	{
		printf("get item DNS error\n");
		return -1;
	}
	m_spider_conf.dns_host = str_result;
    // DNS服务端口
	if ((int_result=conf.get_int_item("DNS_PORT")) <= 0)
	{
		printf("get item DNS_PORT error\n");
		return -1;
	}
	m_spider_conf.dns_port = int_result;
    // DNS查询间隔
	if ((int_result=conf.get_int_item("DNS_SLEEP_INTERVAL")) <= 0)
	{
		printf("get item DNS_SLEEP_INTERVAL error\n");
		return -1;
	}
	m_spider_conf.dns_sleep_interval = int_result;
    // 抽取器的配置文件
	if ((str_result=conf.get_string_item("EXTRACTOR_CONF_PATH")).empty())
	{
		printf("get item EXTRACTOR_CONF_PATH error\n");
		return -1;
	}
	m_spider_conf.extractor_conf_path = str_result;
    // 老识别器的配置文件
	if ((str_result=conf.get_string_item("RECOGNIZER_CONF_PATH")).empty())
	{
		printf("get item RECOGNIZER_CONF_PATH error\n");
		return -1;
	}
	m_spider_conf.recognizer_conf_path = str_result;
    // 选取线程的最小选取间隔
	if ((int_result=conf.get_int_item("MIN_SELECT_INTERVAL")) <= 0)
	{
		printf("get item MIN_SELECT_INTERVAL error\n");
		return -1;
	}
	m_spider_conf.min_select_interval = int_result;
    // 每次选多少条URL
	if ((int_result=conf.get_int_item("SELECT_NUMS_PER_TIME")) <= 0)
	{
		printf("get item SELECT_NUMS_PER_TIME error\n");
		return -1;
	}
	m_spider_conf.select_nums_per_time = int_result;
    // 每轮最少选取数
	if ((int_result=conf.get_int_item("MIN_SELECT_THRESHOLD")) <= 0)
	{
		printf("get item MIN_SELECT_THRESHOLD error\n");
		return -1;
	}
	m_spider_conf.min_select_threshold = int_result;
    // 优先队列的阈值
	if ((int_result=conf.get_int_item("PRIORITY_QUOTA")) <= 0)
	{
		printf("get item PRIORITY_QUOTA error\n");
		return -1;
	}
	m_spider_conf.priority_quota = int_result;
    // 选取时 category所占百分比
	if ((int_result=conf.get_int_item("CATE_PERCENT")) <= 0)
	{
		printf("get item CATE_PERCENT error\n");
		return -1;
	}
	m_spider_conf.cate_percent = int_result;
    // 选取时 item所占百分比
	if ((int_result=conf.get_int_item("ITEM_PERCENT")) <= 0)
	{
		printf("get item ITEM_PERCENT error\n");
		return -1;
	}
	m_spider_conf.item_percent = int_result;
    // 选取队列大小
	if ((int_result=conf.get_int_item("SELECTED_QUEUE_SIZE")) <= 0)
	{
		printf("get item SELECTED_QUEUE_SIZE error\n");
		return -1;
	}
	m_spider_conf.selected_queue_size = int_result;
    // 抓取线程数量
	if ((int_result=conf.get_int_item("WORK_THREAD_NUM")) <= 0)
	{
		printf("get item WORK_THREAD_NUM error\n");
		return -1;
	}
	m_spider_conf.work_thread_num = int_result;
	// 统计输出文件
	if ((str_result=conf.get_string_item("STATIS_FILE_PATH")).empty())
	{
		printf("get item STATIS_FILE_PATH error\n");
		return -1;
	}
	m_spider_conf.statis_file_path = str_result;
	// 转码字表文件
	if ((str_result=conf.get_string_item("CONVERTER_CODE_PATH")).empty())
	{
		printf("get item CONVERTER_CODE_PATH error\n");
		return -1;
	}
	m_spider_conf.converter_code_path = str_result;

	// 强规则配置文件路径
	if ((str_result=conf.get_string_item("STRONG_RULE_CONF_PATH")).empty())
	{
		printf("get item STRONG_RULE_CONF_PATH error\n");
		return -1;
	}
	m_spider_conf.strong_rule_conf_path = str_result;

	return 0;
}

int CSpider::start()
{
	//init

	CSpiderConf& conf = m_spider_conf;

	if (-1 == m_statis.init(conf.statis_file_path.c_str()) || -1 == m_statis.write_message_to_file("start spider!")){
		cerr << "init statis file error , exit!" << conf.statis_file_path << endl;
		return -1;
	}
	
	if (!(mp_cate_output = new CUrlOutput()))
	{
		cerr << "malloc cate output error, exit!" << endl;
		return -1;
	}

    string cate_file = conf.url_output_dir + "/" + CATE_LIST;
	if (0 != mp_cate_output->init(cate_file.c_str()))
	{
		cerr << "init cate output error, exit!" << endl;
		return -1;
	}

	if (!(mp_item_output = new CUrlOutput()))
	{
		cerr << "malloc item output error, exit!" << endl;
		return -1;
	}

    string item_file = conf.url_output_dir + "/" + ITEM_LIST;
	if (0 != mp_item_output->init(item_file.c_str()))
	{
		cerr << "init item output error, exit!" << endl;
		return -1;
	}

	if (!(mp_fail_output = new CUrlOutput()))
	{
		cerr << "malloc fail output error, exit!" << endl;
		return -1;
	}

    string fail_file = conf.url_output_dir + "/ " +  FAIL_LIST;
	if (0 != mp_fail_output->init(fail_file.c_str()))
	{
		cerr << "init fail output error, exit!" << endl;
		return -1;
	}


    if (!(mp_page_output = new CPageOutput()))
	{
		cerr << "malloc page output error, exit" << endl;
		return -1;
	}
	if (0 != mp_page_output->init(&conf))
	{
		cerr << "init page_output error, exit" << endl;
		return -1;
	}

    if (!(mp_selected_queue = new CSelectedQueue()))
	{
		cerr << "malloc selected queue error, exit!" << endl;
		return -1;
	}
	if (0 != mp_selected_queue->init(conf.selected_queue_size))
	{
		cerr << "init selected queue error, exit" << endl;
		return -1;
	}

    if (0 != m_dns_client.init(&conf))
	{
		cerr << "init dns client error, exit!" << endl;
		return -1;
	}

    if (!(mp_cpq=new CUrlPool()))
	{
		cerr << "malloc cpq error, exit!" << endl;
		return -1;
	}
	mp_cpq->set_conf(&conf);
    if (!(mp_coq=new CUrlPool()))
	{
		cerr << "malloc coq error, exit!" << endl;
		return -1;
	}
	mp_coq->set_conf(&conf);
    if (!(mp_ipq=new CUrlPool()))
	{
		cerr << "malloc ipq error, exit!" << endl;
		return -1;
	}
	mp_ipq->set_conf(&conf);
    if (!(mp_ioq=new CUrlPool()))
	{
		cerr << "malloc ioq error, exit!" << endl;
		return -1;
	}
	mp_ioq->set_conf(&conf);

	if (!(mp_level_pool = new CLevelPool()))
	{
		cerr << "malloc level pool error, exit!" << endl;
		return -1;
	}
	if (0 != mp_level_pool->init(&conf))
	{
		cerr << "init level pool error, exit!" << endl;
		return -1;
	}

    m_conf_change_time = -1;
    m_stop_domain_conf_change_time = -1;
    m_seed_change_time = -1;
	m_select_rounds = 0;
	
	pthread_t works[m_spider_conf.work_thread_num];
	pthread_t select;
	if (0 != pthread_create(&select, NULL, select_thread, this)) {
		cerr << "start select thread failed." << endl;
		return -1;
	}

	void* work_ret = NULL;
	for (int i = 0; i < m_spider_conf.work_thread_num; i++) {
		if (0 != pthread_create(works+i, NULL, crawl_thread, this)) {
			cerr << "start work thread " << i << " failed." << endl;
			pthread_join(select, &work_ret);
			for (int x=0;x<i;x++) {pthread_join(works[x],&work_ret);}
			return -1;
		}
	}
	SDLOG_INFO(SP_LOGNAME, "spider start successfully");
	
	pthread_join(select, &work_ret);
	for (int x=0;x<m_spider_conf.work_thread_num;x++) {pthread_join(works[x],&work_ret);}
	SDLOG_INFO(SP_LOGNAME, "spider end working");
	
	return 0;
}
