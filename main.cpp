#include <iostream>
#include <unistd.h>
#include "server.hpp"
#include "ZbDB.hpp"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include<sys/types.h>

#include "client_data_process.hpp"
#include "zmq/SyncService.hpp"
#include "ZB_data_processor.hpp"
#include "syslog/XqLog.hpp"

using namespace std;
using namespace xq;

#define BUF_SIZE 1024
char    *p_map;
int g_server_conn_fd[2];
static void updateAllDevStatus();
char * g_msg = "start run zb comm server";

int main()
{
    XQ_LOG_INFO("%s\n",g_msg );
    p_map = (char *)mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if(-1 == pipe(g_server_conn_fd))
    {
        ERR_EXIT("pipe");
    }
    
    pid_t pid;
    pid = fork();   
    if(pid == -1)
        ERR_EXIT("fork");
    if(pid == 0) {
            startTcpServer();
    }else{
        SyncService srv("127.0.0.1", 5555, &ZMQSyncServiceRecvCB, nullptr);
        thread th1(updateAllDevStatus);
        th1.detach();
        while(1){
            if(*(p_map+1023) != 1)
            {
                continue;
            }
            *(p_map+1023) = 0;
            char *temp_p_map = p_map;

            for( int i=0,j=0;;)
            {
                static int k=0;
                if(temp_p_map[i]=='{')  j++;
                if(temp_p_map[i]=='}')  j--;
                if(j==0)
                {
                    char tempBuf[1024];
                    memset(tempBuf,0,sizeof(tempBuf));
                    memcpy(tempBuf,&temp_p_map[k],i-k+1);
                    std::cout << "tempBuf=" << tempBuf << endl;
                    StoreDataInZbDB(tempBuf);
                    k=i+1;
                                   
                }
                i++;
                if(temp_p_map[i]==0 || temp_p_map[i]=='\0')
                {
                    k=0;
                    break;
                }
            }
        }
    }
}

static void updateAllDevStatus()
{
    ZigbeeDB& myZb = ZigbeeDB::get_instance();
    while (1)
    {
        myZb.UpdateDevStatus();
    }
    
}