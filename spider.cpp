#include "spider.h"
#include "selected_queue.h"
#include "url_output.h"
#include "page_output.h"
#include "sdconf.h"
#include "Http.h"
#include <string>

void* crawl_thread(void* arg)
{
	CSpider* psp = (CSpider*)arg;
	CSpiderConf& conf = psp->m_spider_conf;
	CSelectedQueue& sq = psp->m_selected_queue;
	CUrlOutput& fail_output = psp->m_fail_output;
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

	while (!stopped)
	{
        if (!sq.pop(qi))  // SelectedQueue为空
		{
			page_output.append(NULL, 0, false); // 为了满足即使没有抓到URL也写一个page.list空文件
			usleep(3000);
			continue;
		}

        if (qi.fail_count > conf.MAX_URL_FAIL_COUNT)  // 此URL多次抓取失败，丢弃
		{
            fail_output.append(qi.url, "CRAWL_FAIL");
			continue;
		}
		if (qi.dns_count > conf.MAX_DNS_QUERY_COUNT)  // 此URL无IP，丢弃
		{
			fail_output.append(qi.url, "DNS_FAIL");
			continue;
		}
		ip = ip_pool.get_ip(site);
		if (ip == "NO_IP")
		{
			qi.dns_count++;
			// query ip, insert into ip_pool
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
			printf("url build error\n");
			level_pool.finish(site);
			continue;
		}
		site = uc_url.get_site();

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
			fail_output.append(url, "NOT_FOUND");
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
					// look up ip, and insert into ip_pool
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
			printf("redirect 404\n");
			fail_output.append(url, "NOT_FOUND_REDIRECT");
			continue;
		}

		if (file_length <= 0 && file_length > conf.max_page_len)
		{
			printf("page length invalid\n");
			err_str = "CONTENT_LEN_INVALID (" + file_length + ")";
			fail_output.append(url, err_str.c_str());
			continue;
		}

        // transfer content-encoding
		// base64 encoding

		if (0 != page_output.append())
		{
		}
		level_pool.finish_crawl(site);
	}

	return NULL;
}


FuncRet CSpider::load_conf(const char* conf_path)
{
	sdConf conf;
	string str_result;
	int int_result;

	if (FR_OK != conf.load_conf(conf_path))
	{
		printf("load conf error: %s\n", conf_path);
		return FR_FALSE;
	}

    // 站点最大线程并发度
	if ((int_result=conf.get_int_item("DEFAULT_MAX_CONCURRENT_THREAD_COUNT")) <=0)
	{
		printf("get item DEFAULT_MAX_CONCURRENT_THREAD_COUNT error\n");
		return FR_FALSE;
	}
	m_spider_conf.default_max_concurrent_thread_count= int_result;

    // 站点抓取间隔
	if ((int_result=conf.get_int_item("DEFAULT_SITE_CRAWL_INTERVAL")) < 0)
	{
        printf("get item DEFAULT_SITE_CRAWL_INTERVAL error\n");
		return FR_FALSE;
	}
	m_spider_conf.default_site_crawl_interval = int_result;

    // URL抓取所允许的最多失败次数
	if ((int_result=conf.get_int_item("MAX_URL_FAIL_COUNT")) < 0)
	{
        printf("get item MAX_URL_FAIL_COUNT error\n");
		return FR_FALSE;
	}
	m_spider_conf.max_url_fail_count = int_result;


	return FR_OK;
}