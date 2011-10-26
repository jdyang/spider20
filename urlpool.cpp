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
			sleep(conf.url_queue_empty_sleep_time);
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

