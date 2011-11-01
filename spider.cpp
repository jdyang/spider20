#include <string>
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
