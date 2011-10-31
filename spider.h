#ifndef __FANFAN_SPIDER_H__
#define __FANFAN_SPIDER_H__

#include <string>
#include <vector>
#include <queue>
#include <map>
#include "sse_common.h"
#include "ncarch.h"
#include "ncenums.h"
#include "ncconsts.h"
#include "ncmutex.h"
#include "ncsocket.h"
#include "ncpack.h"
#include "ncthread.h"
#include "util.h"
#include "uc_url.h"

#include <time.h>
#include <string>
#include "spider_common.h"
#include "url_output.h"
#include "selected_queue.h"
#include "page_output.h"
#include "dns_client.h"
#include "level_pool.h"

#include "extractor.h"
#include "recognizer.h"

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
