#include <string>
#include "sse_common.h"
#include "spider.h"
#include "selected_queue.h"
#include "url_output.h"
#include "page_output.h"
#include "sdconf.h"
#include "Http.h"
#include "uc_url.h"
#include "base64.h"

bool stopped = false;

void* crawl_thread(void* arg)
{
	CSpider* psp = (CSpider*)arg;
	CSpiderConf& conf = *(psp->m_spider_conf);
	CSelectedQueue& sq = *(psp->m_selected_queue);
	CUrlOutput& fail_output = *(psp->m_fail_output);
	CPageOutput& page_output = *(psp->m_page_output);
	CDnsClient& dns_client = *(psp->m_dns_client);

	SSQItem qi;

	int file_length = -1;
	int redirect_count = 0;

    char* downloaded_file = NULL;
	char* file_head = NULL;
	char* location = NULL;
	int ncSock = -1;

	ExtraHttpConfig ehconfig;
	ehconfig.proxy_str = NULL;

	string redirect_url;
	string site = "";
	string domain = "";
	string ip = "";

    string converted_content = "";
	string base64_content = "";

	while (!stopped)
	{
        if (!sq.pop(qi))  // SelectedQueue为空
		{
			page_output.append(NULL, 0, false); // 为了满足即使没有抓到网页也写一个page.list空文件
			usleep(conf.selected_queue_empty_sleep_time*1000);
			continue;
		}

        if (qi.fail_count > conf.max_url_fail_count)  // 此URL多次抓取失败，丢弃
		{
            fail_output.append_error(qi.url, "CRAWL_FAIL");
			continue;
		}
		if (qi.dns_count > conf.max_dns_query_count)  // 此URL无IP，丢弃
		{
			fail_output.append_error(qi.url, "DNS_FAIL");
			continue;
		}

		ip = dns_client.get_ip(site);
		if (ip == "NO_IP")
		{
			qi.dns_count++;
			sq.push(qi);
		}

		if (!level_pool.is_crawl_enabled(qi.url))  // 不符合压力控制规则
		{
			sq.push(qi);
			continue;
		}

		url = qi.url;
		ucUrl uc_url(url);
		if (uc_url.build() != FR_OK)
		{
			level_pool.finish_crawl(site, false);
			fail_output.append_error(qi.url, "FORMAT_ERROR");
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

        ehconfig.ip_str = ip.c_str();
	    file_length = http.Fetch(url, &downloaded_file, &file_head, &location, &ncSock, &ehconfig);

		if (-404 == file_length)
		{
			printf("download 404\n");
			fail_output.append_error(url, "NOT_FOUND");
			level_pool.finish_crawl(site);
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
					ehconfig.ip_str = ip.c_str();

				}
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
			level_pool.finish_crawl(site);
			fail_output.append_error(url, "NOT_FOUND_REDIRECT");
			continue;
		}

		if (file_length <= 0 && file_length > conf.max_page_len)
		{
			level_pool.finish_crawl(site);
			err_str = "CONTENT_LEN_INVALID (" + file_length + ")";
			fail_output.append_error(url, err_str.c_str());
			continue;
		}
		level_pool.finish_crawl(site, true);

        if (qi.which_queue == QUEUE_TYPE_CPQ || qi.which_queue)  // category写入cate.list
		{
			cate_output.append(qi.url);
		}
		else                           // item写入item.list
		{
			item_output.append(qi.url);
		}


        utf8_converter.set(downloaded_file, strlen(downloaded_file));
		if (-1 == utf8_converter.to_utf8())
		{
			printf("converted to utf8 error\n");
			continue;
		}
		converted_content = utf8_converter.get_converted_content();

        // extractor links and enter queue

        // write to page list
		if (-1 == write_page_list(p_page_output, url, domain, site, flag, converted_content))
		{
            printf("write page error\n");
			continue;
		}
	}

	return NULL;
}

int CSpider::write_page_list(CPageOutput* pout, string& url, string& domain, string& site, int flag, string& converted_content)
{
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
    memset(url_sign_buf, 0, sizeof(url_sign_buf));
	memset(page_sign_buf, 0, sizeof(page_sign_buf));
    memset(m_pagelist_buf, 0, sizeof(m_pagelist_buf));

    sprintf(time_buf, "%lu", time(NULL));

	sprintf(flag_buf, "%d", flag);

	CSign::Generate(url.c_str(), url.length(), &url_sign1, &url_sign2);
	sprintf(url_sign1_buf, "%u", url_sign1);
	sprintf(url_sign2_buf, "%u", url_sign2);

    base64_content = base64_encode(converted_content.c_str(), converted_content.length());
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
				  + 10                                 // 10 "\t"

    if (total_len >= m_pagelist_buf)
	{
		return -1;
	}

    strcat(m_pagelist_buf, time_buf);
	strcat("\t");
    sprintf(m_pagelist_buf, url.c_str());
	sprintf(m_pagelist_buf, "\t");
	strcat(m_pagelist_buf, domain.c_str());
	strcat(m_pagelist_buf, "\t");
	strcat(m_pagelist_buf, site.c_str());
	strcat(m_pagelist_buf, "\t");
	strcat(m_pagelist_buf, url_sign1_buf);
	strcat(m_pagelist_buf, "\t");
	strcat(m_pagelist_buf, url_sign2_buf);
	strcat(m_pagelist_buf, "\t");
	strcat(m_pagelist_buf, page_sign1_buf);
	strcat(m_pagelist_buf, "\t");
	strcat(m_pagelist_buf, page_sign2_buf);
	strcat(m_pagelist_buf, "\t");
	strcat(m_pagelist_buf, flag_buf);
	strcat(m_pagelist_buf, "\t");
	strcat(m_pagelist_buf, base64_content.c_str());

    if (0 != pout->append(m_pagelist_buf, strlen(m_pagelist_buf)))
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
