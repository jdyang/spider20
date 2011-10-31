#include "dns_client.h"
#include <string>
using namespace std;

void DnsClient::set_conf(CSpiderConf *conf){
	m_conf = conf;
}

CSpiderConf* DnsClient::get_conf()
{
	return m_conf;
}
int DnsClient::init(CSpiderConf *conf)
{
	m_ip_mutex.lock();
	m_ip_list.clear();
	m_ip_mutex.unlock();
	
	set_conf(conf);
	
	return 0;
}

void DnsClient::put_ip(string site, string ip)
{
	m_ip_mutex.lock();
	m_ip_list.insert(make_pair(site, ip));
	m_ip_mutex.unlock();
}

string DnsClient::get_ip(string site)
{
	if (site.length() < 3)
		return "NO_IP";
	string ip;
	if (m_ip_list.find(site) == m_ip_list.end()){
		ip = query_real_dns(site);
		put_ip(site, ip);
	} else {
		ip = m_ip_list[site];
	}
	return ip;
}

int DnsClient::query_site_ip(set *sites)
{
	vector<string>::iterator it;
	string ip;
	for (it = sites.begin(); it != sites.end(); ++it){
		ip = query_real_dns(*it);
		put_ip(*it, ip);
		usleep(100*1000);
	}
	return 0;
}

string DnsClient::query_real_dns(string site)
{
	
	string ret = "NO_IP";
	
	char ip[32];
	
	
	char dns_host[32];
	strncpy(dns_host, m_conf->dns_host.c_str(), strlen(m_conf->dns_host.c_str()));
	
	for (int i = 3; i < 6 && udp_dns.gethostipbyname_r_o(dns_host, get_conf()->dns_port, site.c_str(), ip, i, 0) < 0; ++i){
		usleep(get_conf()->dns_sleep_interval*1000);
		strcpy(ip,"NO_IP");
	}

	return string(ip);
}

