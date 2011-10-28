#ifndef __UDPDNS_H__
#define __UDPDNS_H__

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/in.h>


#define UDP_DNS_PORT	53
#define SITE_NAME_LEN	64
#define UDP_MSGLEN		512

typedef struct dns_header 
{
	unsigned short id; //标识，通过它客户端可以将DNS的请求与应答相匹配；

	//标志：[ QR | opcode | AA| TC| RD| RA | zero | rcode ]
	unsigned short flag_QR:1;
	unsigned short flag_opcode:4;
	unsigned short flag_AA:1;
	unsigned short flag_TC:1;
	unsigned short flag_RD:1;
	unsigned short flag_RA:1;
	unsigned short flag_zero:3;
	unsigned short flag_rcode:4;

	unsigned short quests; //问题数目
	unsigned short answers; //回答数目
	unsigned short author; //授权资源记录数目
	unsigned short addition; //额外资源记录数目
}DNS_HEADER;

typedef struct dns_query
{
	unsigned short type; //查询类型
	unsigned short classes; //查询类,通常是A类既查询IP地址。
}DNS_QUERY;

typedef struct dns_response
{
	unsigned short name; //查询的域名
	unsigned short type; //查询类型
	unsigned short classes; //类型码
	unsigned int ttl; //生存时间
	unsigned short length; //资源数据长度
}DNS_RESPONSE;

typedef enum enum_query_type	//查询的资源记录类型。
{
	QT_A=0x01, //指定计算机 IP 地址。
	QT_NS=0x02, //指定用于命名区域的 DNS 名称服务器。
	QT_MD=0x03, //指定邮件接收站（此类型已经过时了，使用MX代替）
	QT_MF=0x04, //指定邮件中转站（此类型已经过时了，使用MX代替）
	QT_CNAME=0x05, //指定用于别名的规范名称。
	QT_SOA=0x06, //指定用于 DNS 区域的“起始授权机构”。
	QT_MB=0x07, //指定邮箱域名。
	QT_MG=0x08, //指定邮件组成员。
	QT_MR=0x09, //指定邮件重命名域名。
	QT_NULL=0x0A, //指定空的资源记录
	QT_WKS=0x0B, //描述已知服务。
	QT_PTR=0x0C, //如果查询是 IP 地址，则指定计算机名；否则指定指向其它信息的指针。
	QT_HINFO=0x0D, //指定计算机 CPU 以及操作系统类型。
	QT_MINFO=0x0E, //指定邮箱或邮件列表信息。
	QT_MX=0x0F, //指定邮件交换器。
	QT_TXT=0x10, //指定文本信息。
	QT_UINFO=0x64, //指定用户信息。
	QT_UID=0x65, //指定用户标识符。
	QT_GID=0x66, //指定组名的组标识符。
	QT_ANY=0xFF //指定所有数据类型。
} ENUM_QUERY_TYPE; 

typedef enum enum_query_class	//指定信息的协议组。
{
	QC_IN=0x01, //指定 Internet 类别。
	QC_CSNET=0x02, //指定 CSNET 类别。（已过时）
	QC_CHAOS=0x03, //指定 Chaos 类别。
	QC_HESIOD=0x04,//指定 MIT Athena Hesiod 类别。
	QC_ANY=0xFF //指定任何以前列出的通配符。
} ENUM_QUERY_CLASS; 

class CUdpDns{
	const static unsigned short m_query_id = 0x237F; //any number
public:
	CUdpDns(){};
	~CUdpDns() {};
	
	////////////////////////
	//interfaces
	int gethostwithfd_r(int sockfd,struct sockaddr *servaddr,const char *domain,char *ip);
	int gethostwithfd_r_o(int sockfd,struct sockaddr *servaddr,const char *domain,char *ip,int sec,int usec);
	int gethostipbyname_r(char *dnshost,int port,const char *domain,char *ip);
	int gethostipbyname_r_o(char *dnshost,int port,const char *domain,char *ip,int sec,int usec);

private:
	
	///////////////////////
	//methods 
	int udp_connect(char *host,int udp_port, struct sockaddr_in *servaddr);
	int send_recv_msgo(int sockfd, struct sockaddr *pservaddr, char *datamsg, int datalen,int sec,int usec );
	int send_recv_msg( int sockfd , struct sockaddr *pservaddr , char *datamsg, int datalen );
	int set_dns_query(char *datamsg,const char *domain);
	int get_dns_response(char *datamsg,char *ip);
	int get_dns_resonse_head(char *datamsg);
};

#endif
