#include "dns_client.h"
#include <string>
using namespace std;

void CDnsClient::set_conf(CSpiderConf *conf){
	m_conf = conf;
}

CSpiderConf* CDnsClient::get_conf()
{
	return m_conf;
}
int CDnsClient::init(CSpiderConf *conf)
{
	m_ip_mutex.lock();
	m_ip_list.clear();
	m_ip_mutex.unlock();
	
	set_conf(conf);
	
	return 0;
}

int CDnsClient::clear_map()
{	m_ip_mutex.lock();
	m_ip_list.clear();
	m_ip_mutex.unlock();
	return 0;
}

void CDnsClient::put_ip(string site, string ip)
{
	m_ip_mutex.lock();
	m_ip_list.insert(make_pair(site, ip));
	m_ip_mutex.unlock();
}

string CDnsClient::get_ip(const char* site_char)
{
	if (NULL == site_char)
		return "NO_IP";
	string ip;
	string site(site_char);
	m_ip_mutex.lock();
	if (m_ip_list.find(site) == m_ip_list.end()){
		m_ip_mutex.unlock();
		ip = query_real_dns(site_char);
		put_ip(site, ip);
	} else {
		ip = m_ip_list[site];
		m_ip_mutex.unlock();
	}
	
	return ip;
}

int CDnsClient::query_site_ip(set<string> *sites)
{
	set<string>::iterator it;
	string ip;
	for (it = sites->begin(); it != sites->end(); ++it){
		ip = query_real_dns((*it).c_str());
		put_ip(*it, ip);
		usleep(100*1000);
	}
	return 0;
}

//singleton query
string CDnsClient::query_real_dns(const char* site_char)
{
	string ret = "NO_IP";
	
	char ip[32];
	
	
	char dns_host[32];
	strncpy(dns_host, m_conf->dns_host.c_str(), strlen(m_conf->dns_host.c_str()));
	dns_host[strlen(m_conf->dns_host.c_str())] = '\0';	
	
	for (int i = 3; i < 6 ; ++i){
		m_singleton_mutex.lock();
		if (udp_dns.gethostipbyname_r_o(dns_host, m_conf->dns_port, site_char, ip, i, 0) >=0 ){
			m_singleton_mutex.unlock();
			break;
		}
		m_singleton_mutex.unlock();
		usleep(m_conf->dns_sleep_interval*1000);
		strcpy(ip,"NO_IP");
	}

	return string(ip);
}

