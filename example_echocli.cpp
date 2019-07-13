/*
* Tencent is pleased to support the open source community by making Libco available.

* Copyright (C) 2014 THL A29 Limited, a Tencent company. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License"); 
* you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at
*
*	http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, 
* software distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and 
* limitations under the License.
*/
#include "log.h"
#include "list.h"

#include "co_routine.h"
#include <errno.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stack>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>


using namespace std;
struct stEndPoint
{
	char *ip;
	unsigned short int port;
};

static void SetAddr(const char *pszIP,const unsigned short shPort,struct sockaddr_in &addr)
{
	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(shPort);
	int nIP = 0;
	if( !pszIP || '\0' == *pszIP   
			|| 0 == strcmp(pszIP,"0") || 0 == strcmp(pszIP,"0.0.0.0") 
			|| 0 == strcmp(pszIP,"*") 
	  )
	{
		nIP = htonl(INADDR_ANY);
	}
	else
	{
		nIP = inet_addr(pszIP);
	}
	addr.sin_addr.s_addr = nIP;

}

static int iSuccCnt = 0;
static int iFailCnt = 0;
static int iTime = 0;

void AddSuccCnt()
{
	int now = time(NULL);
	if (now >iTime)
	{
		printf("time %d Succ Cnt %d Fail Cnt %d\n", iTime, iSuccCnt, iFailCnt);
		iTime = now;
		iSuccCnt = 0;
		iFailCnt = 0;
	}
	else
	{
		iSuccCnt++;
	}
}
void AddFailCnt()
{
	int now = time(NULL);
	if (now >iTime)
	{
		printf("time %d Succ Cnt %d Fail Cnt %d\n", iTime, iSuccCnt, iFailCnt);
		iTime = now;
		iSuccCnt = 0;
		iFailCnt = 0;
	}
	else
	{
		iFailCnt++;
	}
}

static void *readwrite_routine( void *arg )
{
    printf("readwrite_routine %p\n", arg);
    //return 0;
	co_enable_hook_sys();

	stEndPoint *endpoint = (stEndPoint *)arg;
	char str[8]="sarlmol";
	char buf[ 1024 * 16 ];
	int fd = -1;
	int ret = 0;
	for(int i = 0; i < 3; i++)
	{
		if ( fd < 0 )
		{
			fd = socket(PF_INET, SOCK_STREAM, 0);
			struct sockaddr_in addr;
			SetAddr(endpoint->ip, endpoint->port, addr);
			ret = connect(fd,(struct sockaddr*)&addr,sizeof(addr));
						
			if ( errno == EALREADY || errno == EINPROGRESS )
			{       
				struct pollfd pf = { 0 };
				pf.fd = fd;
				pf.events = (POLLOUT|POLLERR|POLLHUP);
                printf("before co_poll i=%d fd:%d\n", i, fd);
				co_poll( co_get_epoll_ct(),&pf,1,200);
                printf("after co_poll i=%d fd:%d\n", i, fd);
				//check connect
				int error = 0;
				uint32_t socklen = sizeof(error);
				errno = 0;
                printf("before getsockopt i=%d fd:%d\n", i, fd);
				ret = getsockopt(fd, SOL_SOCKET, SO_ERROR,(void *)&error,  &socklen);
                printf("after getsockopt i=%d fd:%d\n", i, fd);
                if ( ret == -1 ) 
				{       
					printf("getsockopt ERROR ret %d %d:%s\n", ret, errno, strerror(errno));
					close(fd);
					fd = -1;
					AddFailCnt();
					continue;
				}       
				if ( error ) 
				{       
					errno = error;
					printf("connect ERROR ret %d %d:%s\n", error, errno, strerror(errno));
					close(fd);
					fd = -1;
					AddFailCnt();
					continue;
				}       
			} 
	  			
		}
		printf("before write i=%d fd:%d\n", i, fd);
		ret = write( fd,str, 8);
        printf("after write i=%d fd:%d ret:%d\n", i, fd, ret);
		if ( ret > 0 )
		{
            printf("before read i=%d fd:%d\n", i, fd);
			ret = read( fd,buf, sizeof(buf) );
            printf("after read i=%d fd:%d ret:%d\n", i, fd, ret);
			if ( ret <= 0 )
			{
				printf("co %p read ret %d errno %d (%s)\n",
						co_self(), ret,errno,strerror(errno));
				close(fd);
				fd = -1;
				AddFailCnt();
			}
			else
			{
				printf("echo %s fd %d\n", buf,fd);
				AddSuccCnt();
			}
		}
		else
		{
			printf("co %p write ret %d errno %d (%s)\n",
					co_self(), ret,errno,strerror(errno));
			close(fd);
			fd = -1;
			AddFailCnt();
		}
	}
	return 0;
}

int main(int argc,char *argv[])
{
    
    //初始化参数
    FLAGS_logtostderr = false;				//TRUE:标准输出,FALSE:文件输出
    FLAGS_alsologtostderr = false;			//除了日志文件之外是否需要标准输出
    FLAGS_colorlogtostderr = false;			//标准输出带颜色
    FLAGS_logbufsecs = 0;					//设置可以缓冲日志的最大秒数，0指实时输出
    FLAGS_max_log_size = 10;				//日志文件大小(单位：MB)
    FLAGS_stop_logging_if_full_disk = true; //磁盘满时是否记录到磁盘
    
    google::InitGoogleLogging(argv[0]);
    google::SetLogDestination(google::GLOG_INFO,"./log/echocli");
    
	stEndPoint endpoint;
	endpoint.ip = argv[1];
	endpoint.port = atoi(argv[2]);
	int cnt = atoi( argv[3] );
	int proccnt = atoi( argv[4] );
	
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction( SIGPIPE, &sa, NULL );
	
	for(int k=0;k<proccnt;k++)
	{
        BUG_ON(true);
        BUG();
		pid_t pid = fork();
		if( pid > 0 )
		{
			continue;
		}
		else if( pid < 0 )
		{
			break;
		}
		for(int i=0;i<cnt;i++)
		{
			stCoRoutine_t *co = 0;
			co_create( &co,NULL,readwrite_routine, &endpoint);
			co_resume( co );
		}
		co_eventloop( co_get_epoll_ct(),0,0 );

		exit(0);
	}
	return 0;
}
/*./example_echosvr 127.0.0.1 10000 100 50*/
