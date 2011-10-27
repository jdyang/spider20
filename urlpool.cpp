#include "urlpool.h"

#include "sdlog.h"


using namespace std;

struct PredatorConf;

//=============== UrlPool ================================

PredatorConf& UrlPool::get_conf() {
	return *m_conf;
}


void UrlPool::set_conf(SpiderConf *conf) {
	m_conf = conf;
}

deque<UrlInfo>& UrlPool::get_url_queue() {
	return m_url_queue;
}

set<string>& UrlPool::get_url_set() {
	return m_url_set;
}

bool UrlPool::url_empty() {
	return m_url_queue.empty();
}


void UrlPool::push_url(UrlInfo ui) {
	
	if(null == ui.url)
		return;
    m_url_pool.m_url_set_mutex.lock();
	if (m_url_set.find(ui.url) == m_url_set.end()){
		m_url_pool.m_url_set.insert(ui.url);
		m_url_pool.m_url_set_mutex.unlock();
		m_url_mutex.lock();
		m_url_queue.push_back(ui);
		m_url_mutex.unlock();
	} else{
		m_url_pool.m_url_set_mutex.unlock();
	}
}

UrlInfo UrlPool::pop_url(void) {
	UrlInfo ui;
    SpiderConf& conf = get_conf();

	while (true) {
		m_url_mutex.lock();
		if (m_url_queue.empty()) {
			m_url_mutex.unlock();
			usleep(conf.urlpool_empty_sleep_time);
			continue;
		}
		ui = m_url_queue.front();
		if(NULL == ui.url || ui.url.length() < 4){
			m_url_mutex.unlock();
			continue;
		}	
		m_url_queue.pop_front();

		m_url_mutex.unlock();
		return ui;
	}
}
FuncRet UrlPool::load_urls(const char *path) {
	deque<UrlInfo>& url_queue = get_url_queue();
	set<string>& url_set = get_url_set();
    SpiderConf& conf = m_conf;

    char url_buf[conf->max_url_len+1];
	int url_len;

    FILE *file = fopen(path, "r");
    if (file == NULL) {
		cerr << "load urls failed" << endl;
        return FR_PROCESSERR;
    }

    while (fgets(url_buf, conf.max_url_len+1, file) != NULL) {
        url_len = strlen(url_buf);
        if (url_buf[url_len-1] == '\n' || url_buf[url_len-1] == '\r') {
            url_buf[url_len-1] = '\0';
        }
        if (url_buf[url_len-2] == '\n' || url_buf[url_len-2] == '\r') {
            url_buf[url_len-2] = '\0';
        }
		if (strlen(url_buf) > 4) {
            UrlInfo ui;
            ui.url.assign(url_buf);
			UcUrl ucUrl(ui.url);
			ucUrl.build();
			ui.domain = ucUrl.get_domain();
			ui.site = ucUrl.get_site();
            ui.last_crawl_time = 0;
            url_queue.push_back(ui);
			url_set.insert(ui.url);
		    url_buf[0] = '\0';
        }
	}
    fclose(file);

	return FR_OK;
}

