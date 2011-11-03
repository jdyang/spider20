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
#include <vector>
#include "spider_common.h"
#include "spider_statis.h"
#include "url_output.h"
#include "selected_queue.h"
#include "page_output.h"
#include "dns_client.h"
#include "level_pool.h"
#include "urlpool.h"
#include "spider_statis.h"

#include "extractor.h"
#include "recognizer.h"

class CSpider
{
public:
    int load_conf(const char* conf_path);
	int init(void);
    void write_to_queue(int which_queue, CExtractor* extractor, CUrlRecognizer* url_recog);

	int start();
    
	int write_page_list(CPageOutput* pout, string& url, string& domain, string& site, int flag, string& converted_content, char* page_list_buf, int page_list_buf_len);
	int select_url();//0 for ok, 1 for next round
	int insert_url();
	int update_conf();
	int next_round();
	
	CSpiderConf m_spider_conf;

    CSelectedQueue* mp_selected_queue;

	CUrlOutput* mp_cate_output;
	CUrlOutput* mp_item_output;
	CUrlOutput* mp_fail_output;
	
	CUrlPool* mp_cpq;
	CUrlPool* mp_ipq;
	CUrlPool* mp_coq;
	CUrlPool* mp_ioq;
	
	//select tmp queue, need to be shuffled 
	vector<UrlInfo> m_select_buffer;
	vector<UrlInfo> m_select_back;
	int m_select_rounds;
	
	CSpiderStatis m_statis;

	CPageOutput* mp_page_output;

	CDnsClient m_dns_client;

	CLevelPool* mp_level_pool;

};

#endif
