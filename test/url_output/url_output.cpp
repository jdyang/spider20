#include "url_output.h"
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

using namespace std;

int CUrlOutput::init(string file_name)
{

	m_fd = -1;

	if (-1 == (m_fd=open(file_name.c_str(), O_CREAT | O_RDWR)))
	{
		printf("open file error.\n");
		return -1;
	}

    if (0 != pthread_mutex_init(&m_mutex, NULL))
	{
		return -1;
	}
	return 0;
}

int CUrlOutput::destroy(void)
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


int CUrlOutput::append(string& url)
{
	if (0 != pthread_mutex_lock(&m_mutex))
	{
		printf("lock fail\n");
	}
	memset(m_buf, 0, sizeof(m_buf));
	sprintf(m_buf, "%s\t", url.c_str());
	sprintf(m_buf, "%llu\n", time(NULL));
	if (-1 == write(m_fd, m_buf, strlen(m_buf)))
	{
		printf("write err\n");
	}

	if (0 != pthread_mutex_unlock(&m_mutex))
	{
		printf("unlock fail\n");
	}

	return 0;
}
