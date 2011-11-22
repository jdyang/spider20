#include "spider_common.h"
#include "page_output.h"
#include <cstring>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

using namespace std;

int CPageOutput::init(CSpiderConf* p_conf)
{
	m_fd = -1;
	m_cur_day = -1;
	m_cur_min = -1;

    mp_conf = p_conf;

    if (0 != pthread_mutex_init(&m_mutex, NULL))
	{
		return -1;
	}
	return 0;
}

int CPageOutput::destroy(void)
{
	if (-1 != m_fd)
	{
		close(m_fd);
		m_fd = -1;
	}

	if (0 != pthread_mutex_destroy(&m_mutex))
	{
		return -1;
	}
	return 0;
}

int CPageOutput::append(const char*page, int len, bool need_write)
{

    CSpiderConf& conf = *mp_conf;

    char buf[4096];

	pthread_mutex_lock(&m_mutex);

	time_t now = time(NULL);
	struct tm* p_now = localtime(&now);

	int year = p_now->tm_year+1900;
	int month = p_now->tm_mon + 1;
	int day = p_now->tm_mday;
	int hour = p_now->tm_hour;
	int min = p_now->tm_min / conf.page_name_change_interval;


    if (day != m_cur_day)
	{
		memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s/%d%02d%02d", conf.page_dir.c_str(), year, month, day);
		m_cur_day = day;
		if ( 0 != access(buf, 0))   // only create dir when not exist
		{
		    if (0 != mkdir(buf, 0777))
		    {
			    pthread_mutex_unlock(&m_mutex);
			    return -1;
		    }
		}
	}

	if (min != m_cur_min)
	{
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%s/%d%02d%02d/page.%s.%d%02d%02d%02d%02d", conf.page_dir.c_str(), year, month, m_cur_day, conf.spider_name.c_str(), year, month, m_cur_day, hour, min*conf.page_name_change_interval);
		if (-1 != m_fd)
		{
			close(m_fd);
			m_fd = -1;
		}
		m_fd = open(buf, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP);
		if (-1 == m_fd)
		{
			pthread_mutex_unlock(&m_mutex);
			return -1;
		}
		m_cur_min = min;
	}
	if (need_write)
	{
		if (-1 == write(m_fd, page, len))
		{
			SDLOG_WARN(SP_WFNAME, "page write error: "<< strerror(errno));
			pthread_mutex_unlock(&m_mutex);
			return -1;
		}
	}

	pthread_mutex_unlock(&m_mutex);

	return 0;
}

