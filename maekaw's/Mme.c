#define MAXPENDING 100
#define REQ 1
#define VOTE 2
#define REL 3
#define INQUIRE 4
#define RELINQUISH 5
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
     int site;
     int pid;
     int portn;
 }buffer;

typedef struct
 { int sock;
    struct sockaddr_in peer;
 }arguments;

struct node {
    int clock;
   int pid;
   int site;
   struct node *next;
 };

typedef struct 
 { int site;
   int pid;
   int have_voted;
   int have_inquired;
   int clock;
 }poll;

//global variables
//-----------------------------
struct node *head=NULL;
buffer sbuffer;
int Yes_count;
poll vote;
int prime_site;
int channelDelay[100];
pthread_mutex_t critical;
pthread_mutex_t queue_head;
pthread_mutex_t sbuffer_global;
unsigned int totalProcesses;
unsigned int processId;
unsigned int ports[10][10];
sem_t mutex;

//-----------------------------

//Function prototypes
//-----------------------------
void Die_with_error(char *errorMessage);
void Enqueue(buffer *rbuffer);
void Dequeue();
void *Handle_TCP_client(void *args1);
void *Recv(void *attribute);
void Send_SES(buffer *rbuffer,int s,int p);
void Send_BCMsg(int msg);
void Intiate_critical();
void MHS(buffer *rbuffer);
void Init_channel_delay();
//----------------------------

void Die_with_error(char *errorMessage){
   perror (errorMessage) ;
   exit(1);
 }

void Enqueue(buffer *rbuffer){
   pthread_mutex_lock(&queue_head);
   struct node *temp = (struct node*) malloc(sizeof(struct node));
    struct node *temp1=NULL;

    struct node *temp2=head;
   temp->clock=rbuffer->pclock;
   temp->pid=rbuffer->pid;
   temp->site=rbuffer->site;
   while(temp2!=NULL && ((temp->clock > temp2->clock) || (temp->clock ==temp2->clock && temp->pid > temp2->pid)))
   {temp1=temp2;
    temp2=temp2->next;
   }
   if(temp1==NULL )
   {
    temp->next = head;
   head = temp;
   pthread_mutex_unlock(&queue_head);
   ////free(rbuffer);
     return ;}

   temp->next =temp1->next;
   temp1->next=temp;
   pthread_mutex_unlock(&queue_head);

   ////free(rbuffer);
   return ;
 }

void Dequeue()
 {struct node *temp=head;
  pthread_mutex_lock(&queue_head);
  if(head!=NULL)
  {
    head=temp->next;
    ////free(temp);
    printf("Dequeued process site no. : %d  ,proc no.: %d\n",temp->site,temp->pid);
    pthread_mutex_unlock(&queue_head);
    return ;
  }
  printf("empty queue\n");
  pthread_mutex_unlock(&queue_head);

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
   if(ports[rbuffer->site][rbuffer->pid]==-1)
    ports[rbuffer->site][rbuffer->pid]=rbuffer->portn;
   //printf("\nReceived Message From:- (%d), site no: %d, proc no.:%d \n", ports[rbuffer->site][rbuffer->pid], rbuffer->site,rbuffer->pid);
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
   unsigned int recvPort = ports[prime_site][processId];
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

void Send_SES(buffer *rbuffer,int s,int p){
   int sendSock;

   struct sockaddr_in peerAddr;
   char *peerlP = "127.0.0.1";
   pthread_mutex_lock(&sbuffer_global);
    sbuffer.pclock++;
    rbuffer->pclock=sbuffer.pclock;
   pthread_mutex_unlock(&sbuffer_global);
   rbuffer->del=channelDelay[p% totalProcesses];
   rbuffer->pid=sbuffer.pid;
   rbuffer->site=sbuffer.site;
   rbuffer->portn=sbuffer.portn;
   int sent=0;

     if ((sendSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
       Die_with_error("socket () failed\n") ;
     memset(&peerAddr, 0, sizeof(peerAddr));
     peerAddr.sin_family = AF_INET;
     peerAddr.sin_addr.s_addr = inet_addr(peerlP);
     peerAddr.sin_port = htons(ports[s][p]);

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
   sendbuffer=sbuffer;
   pthread_mutex_unlock(&sbuffer_global);
   int sent=0;

   for(int p=processId;sent<totalProcesses;p++){
     if ((sendSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
       Die_with_error("socket () failed\n") ;
     memset(&peerAddr, 0, sizeof(peerAddr));
     peerAddr.sin_family = AF_INET;
     peerAddr.sin_addr.s_addr = inet_addr(peerlP);
     peerAddr.sin_port = htons(ports[prime_site][ p % totalProcesses]);
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
    printf("entry section-- REQ is sent with clock %d to all process in site %d\n",sbuffer.pclock+1,prime_site);
    Send_BCMsg(REQ);
    

      // wait till all REP are recived
      while (Yes_count!=totalProcesses)
        {;}
        Yes_count=0;

     printf("all votes  recieved\n");
     

     
  //Critical section------------------
    // pthread_mutex_lock(&critical);
     

     printf("enter critical section\n");
        sleep(3); //critical section
  //-----------------------------------
  //exit critical section--------------
        printf("exit critical section\n");
        printf("exiting critical section- msg sent  REL with clock %d to all process in site %d\n",sbuffer.pclock+1,prime_site);
        Send_BCMsg(REL);//Send relieve message to all processes
        
      // pthread_mutex_unlock(&critical) ;
  //-----------------------------------
    }


void MHS(buffer *rbuffer)
 { 
    pthread_mutex_lock(&sbuffer_global);          
  sbuffer.pclock=sbuffer.pclock > rbuffer->pclock ? (sbuffer.pclock+1):(rbuffer->pclock + 1);
   pthread_mutex_unlock(&sbuffer_global);

   switch(rbuffer->Msg)
   {case REQ:
      {
         printf("MHS - msg recieved REQ with clock %d from  (%d), site no: %d, proc no.:%d \n",rbuffer->pclock, ports[rbuffer->site][rbuffer->pid], rbuffer->site,rbuffer->pid);
         if(!vote.have_voted)
         {vote.clock=rbuffer->pclock;
          vote.pid=rbuffer->pid;
          vote.site=rbuffer->site;
          vote.have_voted=1; 
          rbuffer->Msg=VOTE;
          Send_SES(rbuffer,rbuffer->site,rbuffer->pid);
          printf("MHS - msg VOTE sent with clock %d to  (%d), site no: %d, proc no.:%d \n",rbuffer->pclock, ports[vote.site][vote.pid], vote.site,vote.pid);
         }
         else
         {
          Enqueue(rbuffer);
          if((rbuffer->pclock < vote.clock|| (rbuffer->pclock == vote.clock && rbuffer->pid < vote.pid) )&& vote.have_inquired==0)
          {   rbuffer->Msg=INQUIRE;
            Send_SES(rbuffer,vote.site,vote.pid);
            printf("MHS - msg INQUIRE sent with clock %d to  (%d), site no: %d, proc no.:%d \n",rbuffer->pclock, ports[vote.site][vote.pid], vote.site,vote.pid);
            vote.have_inquired=1;
          }
         }
        
        break;
      }
    case  VOTE:
        {printf("MHS - msg recieved VOTE with clock %d from  (%d), site no: %d, proc no.:%d \n",rbuffer->pclock, ports[rbuffer->site][rbuffer->pid], rbuffer->site,rbuffer->pid);
        Yes_count++;
         break;


        }
    case REL:
        {printf("MHS - msg recieved REL with clock %d from  (%d), site no: %d, proc no.:%d \n",rbuffer->pclock, ports[rbuffer->site][rbuffer->pid], rbuffer->site,rbuffer->pid);
         if(head!=NULL)
         { 
          vote.clock=head->clock;
          vote.pid=head->pid;
          vote.site=head->site;
          rbuffer->Msg=VOTE;
          Dequeue();
          Send_SES(rbuffer,vote.site,vote.pid);
          printf("MHS - msg VOTE sent with clock %d to  (%d), site no: %d, proc no.:%d \n",rbuffer->pclock, ports[vote.site][vote.pid], vote.site,vote.pid);

         }
          
          else 
          {
            vote.have_voted=0;
          }
          vote.have_inquired=0;
          break;

        }

    case INQUIRE:
        { printf("MHS - msg recieved INQUIRE with clock %d from  (%d), site no: %d, proc no.:%d \n",rbuffer->pclock, ports[rbuffer->site][rbuffer->pid], rbuffer->site,rbuffer->pid);
         // pthread_mutex_lock(&critical)  ;
          if(Yes_count!=totalProcesses && Yes_count!=0) //rel and inquire simultaneously,use ses
          { Yes_count--;
            rbuffer->Msg=RELINQUISH;
            int s=rbuffer->site,p=rbuffer->pid;
          Send_SES(rbuffer,s,p);
          printf("MHS - msg RELINQUISH sent with clock %d to  (%d), site no: %d, proc no.:%d \n",rbuffer->pclock, ports[s][p],s,p);
          }
         // pthread_mutex_unlock(&critical);
          break;
        }
    case RELINQUISH:
        { printf("MHS - msg recieved RELINQUISH with clock %d from  (%d), site no: %d, proc no.:%d \n",rbuffer->pclock, ports[rbuffer->site][rbuffer->pid], rbuffer->site,rbuffer->pid);
          rbuffer->pid=vote.pid;
          rbuffer->site=vote.site;
          rbuffer->pclock=vote.clock;
          Enqueue(rbuffer);
          if(vote.pid!=head->pid) //not empty queue before?
         { vote.clock=head->clock;
          vote.pid=head->pid;
          vote.site=head->site;
          rbuffer->Msg=VOTE;
          Send_SES(rbuffer,vote.site,vote.pid);
          printf("MHS - msg VOTE sent with clock %d to  (%d), site no: %d, proc no.:%d \n",rbuffer->pclock, ports[vote.site][vote.pid], vote.site,vote.pid);
          Dequeue();
          vote.have_inquired=0;
          break;
         }
          Dequeue();
          vote.have_inquired=0;
          break;
        }
    }
    
 }

void Init_channel_delay(){
   int delay=6;
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
   for(int i=0;i<10;i++)
    for(int j=0;j<10;j++)
      ports[i][j]=-1;
   printf("Enter the site no\n");
   scanf("%d",&prime_site);

   printf("Enter  Number Of Processes in the set in the site %d:\n",prime_site);
   scanf("%d",&totalProcesses);
   printf("Enter Process Id of Current Process (should be 0 to %d)\n",totalProcesses-1);
   scanf("%d",&processId);
   for(int p=0;p<totalProcesses;p++){
     printf("Enter (Procees-%d) of site %d port number \n",p,prime_site);
     scanf("%d",&ports[prime_site][p]);
   }
   sbuffer.portn=ports[prime_site][processId];
   sbuffer.site=prime_site;
   sbuffer.pid = processId;

   printf("This Procees Port No Is : %d\n",ports[prime_site][processId]);
   vote.have_voted=0;
   Yes_count=0;
   Init_channel_delay();
   

   pthread_t Receive;

   if(pthread_create(&Receive, NULL, Recv, NULL)){
     Die_with_error("Failed To Create Thread To Receive Brodcast Messages");
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
