#define MAXPENDING 10
#define REQ 1
#define REP 2
#define REL 3
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include<semaphore.h>

typedef  struct info {
      int del;
     int pclock;
     int Msg;
     int pid;
 }buffer;

typedef struct
 { int sock;
    struct sockaddr_in peer;
 }arguments;

struct node {
    int clock;
   int pid;
   struct node *next;
 };

//global variables
//-----------------------------
struct node *head=NULL;
buffer sbuffer;
int REP_count;
int channelDelay[10];
pthread_mutex_t critical;
pthread_mutex_t queue_head;
pthread_mutex_t sbuffer_global;
unsigned int totalProcesses;
unsigned int processId;
unsigned int ports[10];
sem_t mutex;
int critical_section=0;
int critical_TS=10;
buffer *Rep_deferred[10];
//-----------------------------

//Function prototypes
//-----------------------------
void Die_with_error(char *errorMessage);
void Enqueue(buffer *rbuffer);
void Dequeue();
void *Handle_TCP_client(void *args1);
void *Recv(void *attribute);
void Send_SES(buffer *rbuffer,int p);
void Send_BCMsg(int msg);
void Intiate_critical();
void MHS(buffer *rbuffer);
void Init_channel_delay();
//----------------------------

void Die_with_error(char *errorMessage){
   perror (errorMessage) ;
   exit(1);
 }





void *Handle_TCP_client(void *args1){
    arguments *args=args1;
   int clntSocket=args->sock;
   struct sockaddr_in peerAddr=args->peer;
   ////free(args);
   buffer *rbuffer=(buffer *)malloc(sizeof(buffer));
   int recvMsgSize;
   if ((recvMsgSize = recv(clntSocket, rbuffer, sizeof(buffer), 0)) < 0)
     Die_with_error("recv() failed");
   sleep(rbuffer->del);
   printf("\nReceived Message From:- %s(%d), proc no. :- %d \n", inet_ntoa(peerAddr.sin_addr), ports[rbuffer->pid], rbuffer->pid);
   sem_wait(&mutex);
   MHS(rbuffer);
   sem_post(&mutex);
   close(clntSocket);
 }

void *Recv(void *attribute){
   int recvSock;
   int peerSock;
   struct sockaddr_in recvAddr;
   struct sockaddr_in peerAddr;
   unsigned int recvPort = ports[processId];
   unsigned int peerLen;
   if ((recvSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
     Die_with_error( "socket () failed\n") ;
   memset(&recvAddr, 0, sizeof(recvAddr));
   recvAddr.sin_family = AF_INET;
   recvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   recvAddr.sin_port = htons(recvPort);
   if (bind(recvSock, (struct sockaddr *)&recvAddr, sizeof(recvAddr)) < 0)
     Die_with_error ("bind() failed\n");
   if (listen(recvSock, MAXPENDING) < 0)
   printf("\nSocket Binded To Recv Messages\n");
   pthread_t t[10];
   for (int i=0;;i++){
     peerLen = sizeof(peerAddr);
     if ((peerSock = accept(recvSock, (struct sockaddr *) &peerAddr,&peerLen)) < 0)
       Die_with_error("accept() failed\n");
      arguments *args=(arguments *)malloc(sizeof(arguments));
      args->peer=peerAddr;
      args->sock=peerSock;

     pthread_create(&t[i%10],NULL,Handle_TCP_client,(void*)args);
   }
 }

void Send_SES(buffer *rbuffer,int p){
   int sendSock;

   struct sockaddr_in peerAddr;
   char *peerlP = "127.0.0.1";
   pthread_mutex_lock(&sbuffer_global);
    sbuffer.pclock++;
    rbuffer->pclock=sbuffer.pclock;
   pthread_mutex_unlock(&sbuffer_global);
   rbuffer->del=channelDelay[p% totalProcesses];
   rbuffer->Msg=REP;
   rbuffer->pid=sbuffer.pid;


   int sent=0;

     if ((sendSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
       Die_with_error("socket () failed\n") ;
     memset(&peerAddr, 0, sizeof(peerAddr));
     peerAddr.sin_family = AF_INET;
     peerAddr.sin_addr.s_addr = inet_addr(peerlP);
     peerAddr.sin_port = htons(ports[ p % totalProcesses]);

     if (connect(sendSock, (struct sockaddr *) &peerAddr, sizeof(peerAddr)) < 0)
       Die_with_error("connect () failed\n");
     if (send(sendSock,rbuffer, sizeof(buffer), 0) != sizeof(buffer))
       Die_with_error("send() sent a different number of bytes than expected\n");


 }
 void Send_BCMsg(int msg){
   buffer sendbuffer;
   int sendSock;
   struct sockaddr_in peerAddr;
   char *peerlP = "127.0.0.1";
   pthread_mutex_lock(&sbuffer_global);
   sbuffer.pclock++;
   sbuffer.Msg=msg;
   if (msg==REQ)
    critical_TS=sbuffer.pclock;
   sendbuffer=sbuffer;
   pthread_mutex_unlock(&sbuffer_global);
   int sent=0;

   for(int p=processId+1;sent<totalProcesses-1;p++){
     if ((sendSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
       Die_with_error("socket () failed\n") ;
     memset(&peerAddr, 0, sizeof(peerAddr));
     peerAddr.sin_family = AF_INET;
     peerAddr.sin_addr.s_addr = inet_addr(peerlP);
     peerAddr.sin_port = htons(ports[ p % totalProcesses]);
      sendbuffer.del=channelDelay[p % totalProcesses];
     if (connect(sendSock, (struct sockaddr *) &peerAddr, sizeof(peerAddr)) < 0)
       Die_with_error("connect () failed\n");
     if (send(sendSock, &sendbuffer, sizeof(buffer), 0) != sizeof(buffer))
       Die_with_error("send() sent a different number of bytes than expected\n");
     sent++;
   }
 }

void Intiate_critical()
 {

  // Entry section----------------------
    critical_section++;
    printf("Intiate_critical - msg sent  REQ with clock %d\n",sbuffer.pclock+1);
    Send_BCMsg(REQ);

      // wait till all REP are recived
      while (REP_count!=totalProcesses-1 )
        {;}
        REP_count=0;

     printf("all REP recieved\n");
     



      
  //-----------------------------------
  //Critical section------------------
     


     printf("enter critical section\n");
        sleep(3); //critical section
  //-----------------------------------
  //exit critical section--------------
      printf("exit critical section\n");
      for(int i=0;i<totalProcesses;i++)
        {
          if(Rep_deferred[i]!=NULL)
          { 
            Send_SES(Rep_deferred[i],i);

           printf("Exit CS - msg sent  REP with clock %d\n",Rep_deferred[i]->pclock); 
           Rep_deferred[i]=NULL;
          }
        }
        //(REL);//Send relieve message to all processes
        
        critical_section--;
     
  //-----------------------------------
    }


void MHS(buffer *rbuffer)
 { 
    pthread_mutex_lock(&sbuffer_global);          //if this placed first - posssiblity of deadlock
  sbuffer.pclock=sbuffer.pclock > rbuffer->pclock ? (sbuffer.pclock+1):(rbuffer->pclock + 1);
  pthread_mutex_unlock(&sbuffer_global);

    if(rbuffer->Msg==REQ)
    {
       printf("MHS - msg recieved REQ with clock %d\n",rbuffer->pclock);

       int priority=critical_section && ( critical_TS<rbuffer->pclock || (critical_TS==rbuffer->pclock && processId < rbuffer->pid));
       
       if(priority)
       {
        Rep_deferred[rbuffer->pid]=rbuffer;
       }
       else
       {Rep_deferred[rbuffer->pid]=NULL;
        Send_SES(rbuffer,rbuffer->pid);
        printf("MHS - msg sent  REP with clock %d\n",rbuffer->pclock);
       }
      
    }
    else if( rbuffer->Msg==REP)
    {printf("MHS - msg recieved REP with clock %d\n",rbuffer->pclock);

      REP_count++;
    }
    
    
 }

void Init_channel_delay(){
   int delay=2;
   channelDelay[processId] = 0;
   int chupdated=1;
   for(int ch=processId+1;chupdated<totalProcesses;ch++){
     channelDelay[ ch % totalProcesses ] = delay;
     delay+=8;
     chupdated++;
   }

   printf("Channel Delays to Process %d to %d ----> ",0,totalProcesses-1);
   for(int ch =0 ;ch<totalProcesses;ch++){
     printf("%d,",channelDelay[ch]);
   }
   printf("\n");
 }

int main(int argc, const char * argv[])
 { sem_init(&mutex,0,1)  ;
   pthread_mutex_init(&critical, NULL);
   pthread_mutex_init(&sbuffer_global, NULL);
   pthread_mutex_init(&queue_head, NULL);


   printf("Enter Total Number Of Processes (< 10):\n");
   scanf("%d",&totalProcesses);
   printf("Enter Process Id of Current Process (should be 0 to %d)\n",totalProcesses-1);
   scanf("%d",&processId);
   for(int p=0;p<totalProcesses;p++){
    Rep_deferred[p]=NULL;
     printf("Enter (Procees-%d) Port To Recieve   Message\n",p);
     scanf("%d",&ports[p]);
   }

   sbuffer.pid = processId;

   printf("This Procees Port No Is : %d\n",ports[processId]);
   Init_channel_delay();

   pthread_t Receive;

   if(pthread_create(&Receive, NULL, Recv, NULL)){
     Die_with_error("Failed To Create Thread To Receive  Messages");
       }
   int y;
   int src,dest;



   for(;;){
      y=0;
     printf("\nPress 1 for this process to execute critical section\n");
     scanf("%d" ,&y);
     if(y)
     Intiate_critical();

 }
}
