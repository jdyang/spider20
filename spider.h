#ifndef __FANFAN_SPIDER_H__
#define __FANFAN_SPIDER_H__

#include <time.h>
#include <string>
#include "url_output.h"
#include "selected_queue.h"
#include "page_output.h"

#include "spider_common.h"

class CSpider
{
public:
    int load_conf(const char* conf_path);
	int init(void);

	CSpiderConf m_spider_conf;

    CSelectedQueue* m_selected_queue;

	CUrlOutput* mp_cate_output;
	CUrlOutput* mp_item_output;
	CUrlOutput* mp_fail_output;

	CPageOutput* mp_page_output;
};

#endif
