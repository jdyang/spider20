#include "page_output.h"
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

using namespace std;

int CPageOutput::init(string dir)
{
	m_fd = -1;
	m_cur_day = -1;
	m_cur_min = -1;
	m_base_dir = dir;

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

void CPageOutput::set_spider(CSpider* sp)
{
	mp_spider = sp;
}

int CPageOutput::append(const char*page, int len, bool need_write)
{

    //CSpiderConf& conf = mp_spider->m_spider_conf;

    char buf[4096];

	if (0 != pthread_mutex_lock(&m_mutex))
	{
		printf("lock fail\n");
		return -1;
	}
	time_t now = time(NULL);
	struct tm* p_now = localtime(&now);

	int year = p_now->tm_year+1900;
	int month = p_now->tm_mon;
	int day = p_now->tm_mday;
	int hour = p_now->tm_hour;
	int min = p_now->tm_min / 30;


    if (day != m_cur_day)
	{
		memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s/%d%02d%02d", m_base_dir.c_str(), year, month, day);
		m_cur_day = day;
		if (0 != mkdir(buf, 0777))
		{
			printf("mkdir error\n");
			return -1;
		}
	}

	if (min != m_cur_min)
	{
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%s/%d%02d%02d/page.list.%d%02d%02d%02d%02d", m_base_dir.c_str(), year, month, m_cur_day, year, month, m_cur_day, hour, min*30);
		if (-1 == m_fd)
		{
			close(m_fd);
			m_fd = -1;
		}
		m_fd = open(buf, O_RDWR | O_APPEND | O_CREAT);
		if (-1 == m_fd)
		{
			printf("open file err\n");
			return -1;
		}
		m_cur_min = min;
	}
	if (need_write)
	{
		if (-1 == write(m_fd, page, len))
		{
			printf("write err\n");
			return -1;
		}
	}

	pthread_mutex_unlock(&m_mutex);

	return 0;
}

