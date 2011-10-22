#include "page_output.h"
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

CPageOutput::CPageOutput(string dir)
{
	m_fd = -1;
	m_cur_day = -1;
	m_cur_min = -1;
	m_base_dir = dir;
    m_mutex = PTHREAD_MUTEX_INITALIZE;
}

int CPageOutput::append(const char*page, int len, bool need_write=true)
{

	if (0 != pthread_mutex_lock(&m_mutex))
	{
	}
	int now = time(NULL);
	struct tm* now_tm = localtime(now);

	int year = now_tm.tm_year+1900;
	int month = now_tm.tm_month;
	int day = now_tm.tm_mday;
	int hour = now_tm.tm_hour;
	int min = now_tm.tm_min / 30;


    if (day != m_cur_day)
	{
		memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s/%d%02d%02d", m_base_dir.c_str(), year, month, day);
		m_cur_day = day;
		if (0 != mkdir(buf, 0x777))
		{
		}
	}

	if (min != cur_min)
	{
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%s/%d%02d%02d/page.list.%d%02d%02d%02d%02d", m_base_dir.c_str(), year, month, m_cur_day, year, month, m_cur_day, hour, min*30);
		if (-1 == m_fd)
		{
			close(m_fd);
			m_fd = -1;
		}
		m_fd = open(buf, "a");
		if (-1 == m_fd)
		{
		}
		m_cur_min = min;
	}
	if (need_write)
	{
		return write(m_fd, page, len);
	}

	if (0 != pthread_mutex_unlock(&m_mutex))
	{
	}

	return 0;
}
