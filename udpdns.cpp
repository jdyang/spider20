#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/in.h>

#include <string.h>
#include <errno.h>

#include <iostream>

#include "udpdns.h"

using namespace std;

/*
  ** connect to udp server
  ** argument: host / port  --- for udp server
*/
int CUdpDns::udp_connect(char *host,int udp_port,struct sockaddr_in *servaddr )
{
    int sockfd ;

    memset(servaddr, 0 , sizeof ( struct sockaddr_in )) ;
    servaddr->sin_family = AF_INET ;
    inet_pton ( AF_INET , host , &servaddr->sin_addr ) ;
    servaddr->sin_port = htons ( udp_port );

   	if ( (sockfd = socket(AF_INET, SOCK_DGRAM ,IPPROTO_UDP) ) == -1 )
        return -1 ;

    return sockfd ;
}


/*
 ** send a udp packet to server and receive the response
 ** return value is the length of the response packet.
*/
int CUdpDns::send_recv_msg( int sockfd , struct sockaddr *pservaddr , char *datamsg, int datalen )
{
    int  recvlen ;
    int  sendlen ;
    socklen_t servlen ;

    servlen = (socklen_t ) sizeof ( struct sockaddr_in ) ;
    sendlen = sendto ( sockfd , datamsg , datalen , 0 , pservaddr, servlen ) ;
    if ( sendlen < 0 )
        return -1;
 	
    recvlen  = recvfrom( sockfd , datamsg ,UDP_MSGLEN ,0 ,NULL ,NULL ) ;
    if ( recvlen >= 0 )
    {
        *( datamsg+recvlen) = '\0';
        return recvlen ;
    }
    else
    {
        return -1 ;
    }
}

/*
 ** send a udp packet to server ans receive the response with overtime handling.
*/
int CUdpDns::send_recv_msgo( int sockfd , struct sockaddr *pservaddr , char *datamsg, int datalen ,int sec , int usec )
{
    int  recvlen = 0 ;
    int  sendlen = 0 ;
    socklen_t servlen ;

    servlen = (socklen_t ) sizeof ( struct sockaddr_in ) ;
    sendlen = sendto ( sockfd , datamsg , datalen , 0 , pservaddr, servlen ) ;
    if ( sendlen < 0 )
    {
        return -1;
    }

    fd_set  rset;
    struct timeval  tv ;
    FD_ZERO(&rset);
    FD_SET(sockfd, &rset );

    tv.tv_sec = sec ;
    tv.tv_usec = usec;

    int retval = -1 ;
    retval = select ( sockfd +1 , &rset , NULL,NULL ,&tv );

    if ( retval > 0 )
    {
        recvlen  = recvfrom( sockfd ,  datamsg , UDP_MSGLEN , 0 , NULL , NULL ) ;
        if ( recvlen >= 0 )
        {
            *( datamsg+recvlen) = '\0';
            return recvlen ;
        }
        else
        {
            return -1 ;
        }
    }
    else
        recvlen = -1 ;

    return recvlen ;
}

/*
 ** get the ip of a domain by udp socket descriptor
 ** the return value was stored in the variable -ip-
 ** -1 means that the function was failed.
*/
int CUdpDns::gethostwithfd_r(int sockfd , struct sockaddr *servaddr,const char *domain, char *ip )
{
    if ( sockfd < 0 || servaddr == NULL )
        return -1 ;

    if ( inet_addr(domain) != INADDR_NONE )
    {
        strcpy( ip , domain );
        return 0;
    }

    char sendbuf[ UDP_MSGLEN+1] ;
    memset ( sendbuf , 0 ,  UDP_MSGLEN+1 );

	int query_len =  set_dns_query(sendbuf,domain);
	if (query_len < 0) 
	{
		*ip = 0;
		return -1;
	}

    int recvlen = send_recv_msg ( sockfd , ( struct sockaddr *)&servaddr , sendbuf ,  UDP_MSGLEN ) ;

    if ( recvlen <= 0)
	{
        *ip = '\0' ;
	}
    else 
	{
		int res_len = get_dns_response(sendbuf,ip);
		if (res_len < 0)
		{
			*ip = '\0';
		}
    }
    close ( sockfd ) ;

    if ( ip[0] == 0 )
        return -1;

    return 0 ;
}

/*
 ** get the ip of a domain with overtime handling by udp socket descriptor.
 ** the return value was stored in the variable -ip-
 ** -1 means that the function was failed.
*/
int CUdpDns::gethostwithfd_r_o(int sockfd , struct sockaddr *servaddr ,const char *domain, char *ip , int sec , int usec)
{
    if ( sockfd < 0 || servaddr == NULL )
        return -1 ;

    if ( inet_addr(domain) != INADDR_NONE )
    {
        strcpy( ip , domain );
        return 0;
    }

    char sendbuf[ UDP_MSGLEN+1] ;
    memset ( sendbuf , 0 ,  UDP_MSGLEN+1 );

	int query_len =  set_dns_query(sendbuf,domain);
	if (query_len < 0) 
	{
		*ip = 0;
		return -1;
	}

    int recvlen = send_recv_msgo (sockfd,(struct sockaddr *)&servaddr,sendbuf,UDP_MSGLEN,sec,usec);
    if ( recvlen <= 0)
	{
        *ip = '\0' ;
	}
    else 
	{
		int res_len = get_dns_response(sendbuf,ip);
		if (res_len < 0)
		{
			*ip = '\0';
		}
    }
    close ( sockfd ) ;

    if ( ip[0] == 0 )
        return -1;

    return 0 ;
}
                  
/*
 ** get ip of domain by dns server host and udp listen port.
 ** -1 mean failed. 
*/
int CUdpDns::gethostipbyname_r(char *dnshost,int port,const char *domain,char *ip)
{
    int sockfd ;
    struct sockaddr_in servaddr ;

    if ( inet_addr(domain) != INADDR_NONE )
    {
        strcpy( ip , domain );
        return 0;
    }

    if ( ( sockfd = udp_connect( dnshost , port , &servaddr )  ) == -1 )
        return -1 ;

    char sendbuf[ UDP_MSGLEN+1] ;
    memset ( sendbuf , 0 ,  UDP_MSGLEN+1 );

	int query_len =  set_dns_query(sendbuf,domain);
	/*
	char *cp = sendbuf;
	for (int i=0;i<query_len;i++){
		printf("%2x ",(unsigned char)*cp);
		cp++;
	}
	printf("\n");
	*/
	if (query_len < 0) 
	{
		*ip = 0;
		return -1;
	}

    int recvlen = send_recv_msg ( sockfd , ( struct sockaddr *)&servaddr , sendbuf ,  UDP_MSGLEN ) ;

    //if ( recvlen !=  UDP_MSGLEN )
    if ( recvlen <= 0)
	{
        *ip = '\0' ;
	}
    else 
	{
		/*
		cp = sendbuf;
		for (int i=0;i<recvlen;i++){
			printf("%x ",(unsigned char)*cp);
			cp++;
		}
		printf("\n");
		*/

		int res_len = get_dns_response(sendbuf,ip);
		if (res_len < 0)
		{
			*ip = '\0';
		}
    }
    close ( sockfd ) ;

    if ( ip[0] == 0 )
        return -1;

    return 0 ;
}

/*
 ** get ip of domain by dns server host and udp listen port with overtime handling.
 ** -1 mean failed. 
*/
int CUdpDns::gethostipbyname_r_o(char *dnshost,int udp_port,const char *domain,char *ip,int sec,int usec)
{
    int sockfd ;
    struct sockaddr_in servaddr ;

    if ( inet_addr(domain) != INADDR_NONE )
    {
        strcpy( ip , domain );
        return 0;
    }

    if ( ( sockfd = udp_connect( dnshost , udp_port , &servaddr )  ) == -1 )
        return -1 ;

    char sendbuf[ UDP_MSGLEN+1] ;
    memset ( sendbuf , 0 ,  UDP_MSGLEN+1 );

	int query_len =  set_dns_query(sendbuf,domain);
	if (query_len < 0) 
	{
		*ip = 0;
		return -1;
	}

    int recvlen = send_recv_msgo (sockfd,(struct sockaddr *)&servaddr,sendbuf,UDP_MSGLEN,sec,usec);
    if ( recvlen <= 0)
	{
        *ip = '\0' ;
	}
    else 
	{
		int res_len = get_dns_response(sendbuf,ip);
		if (res_len < 0)
		{
			*ip = '\0';
		}
    }
    close ( sockfd ) ;

    if ( ip[0] == 0 )
        return -1;

    return 0 ;
}


/*
 * set DNS query
*/

int CUdpDns::set_dns_query(char *datamsg,const char *domain)
{
	DNS_HEADER dns_header;
	dns_header.id = m_query_id; //any id

	dns_header.flag_QR  = 0;
	dns_header.flag_opcode = 0;
	dns_header.flag_AA = 1;
	dns_header.flag_TC = 0;
	dns_header.flag_RD = 0;
	dns_header.flag_RA = 1;
	dns_header.flag_zero = 0;
	dns_header.flag_rcode = 0;

	dns_header.quests = 1; //only one question
	dns_header.answers = 0;
	dns_header.author = 0;
	dns_header.addition = 0;
	
	//字节序转换
	for (unsigned int i=0;i<sizeof(DNS_HEADER)/sizeof(short int);i++)
	{
		short int tmp_val = htons(*((short int *)&dns_header + i));
		memcpy(datamsg + i * sizeof(short int),&tmp_val,sizeof(short int));
	}

	//////////////////////////////////////////////
	//set question
	//domain in the form like "www.fanfan.com"  is formatted as "3www6fanfan3com"
	char *cp_pos = datamsg + sizeof(DNS_HEADER);

	const char *cp1 = domain;
	const char *cp2 = cp1;
	int len = 0;
	while (*cp1 != '\0')
	{
		cp2 = cp1;
		while (*cp2 != '.' && *cp2 != '\0')
			cp2++;
		len = cp2 - cp1;
		*cp_pos++ = len;
		for (int i=0;i<len;i++)
		{
			*cp_pos++ = *cp1++;
		}
		if (*cp1 == '\0')
			break;
		cp1++;
	}//end while
	*cp_pos++ = '\0'; //end with 0
	
	//////////////////////////////////////////
	//set dns_query
	DNS_QUERY dns_query;
	dns_query.type = htons(1);	//domain to IPv4 address
	dns_query.classes = htons(1);	//Internet Domain
	
	memcpy(cp_pos,&dns_query,sizeof(DNS_QUERY));
	int ret_len = cp_pos - datamsg + sizeof(DNS_QUERY);

	return ret_len;
}

/*
 * get IP from DNS reponse
 */
int CUdpDns::get_dns_response(char *datamsg,char *ip)
{
	char* cp = datamsg;

	int answer_count = get_dns_resonse_head(cp);
	if (answer_count < 0)
	{
		*ip = 0;
		return -1;
	}
	
	//忽略question
	cp += 12;
	while (*cp != '\0')
		cp++;

	//忽略 query
	cp++;
	cp += 4; //real length of DNS_QUERY
	
	/////////////////////////////////////////////
	//response answer

	ENUM_QUERY_TYPE en_qt = QT_ANY;
	ENUM_QUERY_CLASS en_qc = QC_ANY;

	int count = 0;
	unsigned short int len = 0;

	//get the first record while  QT_A and QC_IN
	while (count < answer_count)
	{
		cp += 2; //name ignored
		unsigned short int* p_qt = (unsigned short int*) cp;

		cp += 2;
		unsigned short int* p_qc = (unsigned short int*) cp;

		cp += 2; 
		cp += 4; //ttl ignored
		unsigned short int* p_len = (unsigned short int*) cp;

		cp += 2;
		en_qt = (ENUM_QUERY_TYPE) ntohs(*p_qt);
		en_qc = (ENUM_QUERY_CLASS) ntohs(*p_qc);
		len = ntohs(*p_len);

		if (en_qt == QT_A && en_qc == QC_IN && 4 == len)
			break;
		
		//next record
		cp += len;
		count++;
	}
	
	if (count == answer_count) 
	{
		*ip = 0;
		return -2;
	}

	int int_ip = ntohl(*((int*)cp));
	int ip1 = (int_ip & 0xFF000000) >> 24;
	int ip2 = (int_ip & 0xFF0000) >> 16;
	int ip3 = (int_ip & 0xFF00) >> 8;
	int ip4 = int_ip & 0xFF;

	int ret_len = sprintf(ip,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);
	return ret_len;
}

/*
 * 分析DNS RESPONSE报文头
 * < 0 格式错误
 * > 0 回答数目
*/
int CUdpDns::get_dns_resonse_head(char *datamsg)
{
	char *cp = datamsg;

	unsigned short* p_msg_id = (unsigned short*) cp;
	unsigned short msg_id = ntohs(*p_msg_id);
	if (msg_id != m_query_id)
		return -1;

	//flag_QR
	cp += 2;
	if ((*cp & 0x80 ) != 0x80){
		cout << "CUdpDns::get_dns_resonse_head() flag_QR != 1" << endl;
		return -2;
	}
	
	//flag_rcode
	cp += 2;
	if ((*cp & 0x0F) != 0){
		cout << "CUdpDns::get_dns_resonse_head() flag_rcode != 0" << endl;
		return -3;
	}
	
	//answer count
	cp += 2;
	unsigned short* p_answer_count = (unsigned short*) cp;
	unsigned short answer_count = ntohs(*p_answer_count);
	if (answer_count <= 0)
		return -4;

	return answer_count;
}

