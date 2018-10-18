#define MAXPENDING 100  
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

 
 int totalProcesses;  
 int processId;  
 unsigned int ports[100];  
 int proccessId;  
  sem_t mutex;
  pthread_mutex_t critical;

 pthread_mutex_t sbuffer_global;
 pthread_cond_t queue_ready;
 int REP_count;
 int channelDelay[100];  
 int delivBuf[100][100];  
 int delivBufIndex=0;  
 

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

 struct node *head=NULL;  
 
buffer sbuffer;
void MHS(buffer *rbuffer);
 void DieWithError(char *errorMessage){  
   perror (errorMessage) ;  
   exit(1);  
 }  
 void addToBuffer(buffer *rbuffer){  
   struct node *temp = (struct node*) malloc(sizeof(struct node));  
    struct node *temp1=NULL;
    struct node *temp2=head;
   temp->clock=rbuffer->pclock;
   temp->pid=rbuffer->pid;
   
   while(temp2!=NULL && ((temp->clock > temp2->clock) || (temp->clock ==temp2->clock && temp->pid > temp2->pid)))
   {temp1=temp2;
    temp2=temp2->next;
   }
   if(temp1==NULL )
   {temp->next = head;  
   head = temp;  
   ////free(rbuffer);
     return ;}
   temp->next =temp1->next;  
   temp1->next=temp;
   
    
   ////free(rbuffer);
   return ;
 } 
 void dequeue()
 {struct node *temp=head;
  if(head!=NULL)  
  {
    head=temp->next;
    ////free(temp);
    printf("dequeued %d\n",temp->pid);
    return ;
  }
  printf("empty queue\n");

 } 

void *handleTCPClient(void *args1){ 
    arguments *args=args1; 
   int clntSocket=args->sock;
   struct sockaddr_in peerAddr=args->peer;
   ////free(args);
   buffer *rbuffer=(buffer *)malloc(sizeof(buffer));
   int recvMsgSize;  
   if ((recvMsgSize = recv(clntSocket, rbuffer, sizeof(buffer), 0)) < 0)  
     DieWithError("recv() failed");  
   sleep(rbuffer->del);  
   printf("\nReceived Message From:- %s(%d), proc no. :- %d \n", inet_ntoa(peerAddr.sin_addr), ports[rbuffer->pid], rbuffer->pid);  
   sem_wait(&mutex);
   MHS(rbuffer);
   sem_post(&mutex);
   close(clntSocket);  
 }  
 void *recvSES(void *attribute){  
   int recvSock;  
   int peerSock;  
   struct sockaddr_in recvAddr;  
   struct sockaddr_in peerAddr;  
   unsigned int recvPort = ports[processId];  
   unsigned int peerLen;  
   if ((recvSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)  
     DieWithError( "socket () failed\n") ;  
   memset(&recvAddr, 0, sizeof(recvAddr));  
   recvAddr.sin_family = AF_INET;  
   recvAddr.sin_addr.s_addr = htonl(INADDR_ANY);  
   recvAddr.sin_port = htons(recvPort);  
   if (bind(recvSock, (struct sockaddr *)&recvAddr, sizeof(recvAddr)) < 0)  
     DieWithError ("bind() failed\n");  
   if (listen(recvSock, MAXPENDING) < 0)  
   printf("\nSocket Binded To Recv Messages\n");  
   pthread_t t[10];
   for (int i=0;;i++){  
     peerLen = sizeof(peerAddr);  
     if ((peerSock = accept(recvSock, (struct sockaddr *) &peerAddr,&peerLen)) < 0)  
       DieWithError("accept() failed\n");
      arguments *args=(arguments *)malloc(sizeof(arguments));
      args->peer=peerAddr;
      args->sock=peerSock;

     pthread_create(&t[i%10],NULL,handleTCPClient,(void*)args);  
   }  
 }  
 void sendSES(buffer *rbuffer,int p){  
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
       DieWithError("socket () failed\n") ;  
     memset(&peerAddr, 0, sizeof(peerAddr));  
     peerAddr.sin_family = AF_INET;  
     peerAddr.sin_addr.s_addr = inet_addr(peerlP);  
     peerAddr.sin_port = htons(ports[ p % totalProcesses]);  
     
     if (connect(sendSock, (struct sockaddr *) &peerAddr, sizeof(peerAddr)) < 0)  
       DieWithError("connect () failed\n");  
     if (send(sendSock,rbuffer, sizeof(buffer), 0) != sizeof(buffer))  
       DieWithError("send() sent a different number of bytes than expected\n");  
    
   
 }
 void sendBCMsg(int msg){  
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
   
   for(int p=processId+1;sent<totalProcesses-1;p++){  
     if ((sendSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)  
       DieWithError("socket () failed\n") ;  
     memset(&peerAddr, 0, sizeof(peerAddr));  
     peerAddr.sin_family = AF_INET;  
     peerAddr.sin_addr.s_addr = inet_addr(peerlP);  
     peerAddr.sin_port = htons(ports[ p % totalProcesses]);  
      sendbuffer.del=channelDelay[p % totalProcesses];  
     if (connect(sendSock, (struct sockaddr *) &peerAddr, sizeof(peerAddr)) < 0)  
       DieWithError("connect () failed\n");  
     if (send(sendSock, &sendbuffer, sizeof(buffer), 0) != sizeof(buffer))  
       DieWithError("send() sent a different number of bytes than expected\n");  
     sent++;  
   }  
 }    

 void Entry_section()
 {
  
  //add to the input queue

  sendBCMsg(REQ);
  addToBuffer(&sbuffer);
  printf("MHS - msg sent  REQ with clock %d\n",sbuffer.pclock);

// wait till all REP are recived
    while (REP_count!=totalProcesses-1 )
      {;}
      REP_count=0;

   printf("all REP recieved\n");
   struct node *temp=head;

   printf("Input queue is:\n");
   while(temp!=NULL)
    {printf("%d|",temp->pid);
    temp=temp->next;}
    printf("\n");
   


   // wait till the process is top of the queue 
    while(head->pid!=processId)
      {;}
    
   pthread_mutex_lock(&critical);
    printf("The request for critical section is in top of queue\n");
      //critical section
   printf("enter critical section\n");
      sleep(3);
      //exit critical section
    printf("exit critical section\n");
      dequeue();
      sendBCMsg(REL);
      printf("MHS - msg sent  REL with clock %d\n",sbuffer.pclock);
     pthread_mutex_unlock(&critical) ;

    }
 

 void MHS(buffer *rbuffer)
 { pthread_mutex_lock(&critical)  ;
    pthread_mutex_lock(&sbuffer_global);          //if his placed first - posssiblity of deadlock
  sbuffer.pclock=sbuffer.pclock > rbuffer->pclock ? (sbuffer.pclock+1):(rbuffer->pclock + 1);
   pthread_mutex_unlock(&sbuffer_global);
    if(rbuffer->Msg==REQ)
    { //add the requesting pid to the queue
       printf("MHS - msg recieved REQ with clock %d\n",rbuffer->pclock);
      addToBuffer(rbuffer);
      sendSES(rbuffer,rbuffer->pid);
      printf("MHS - msg sent  REP with clock %d\n",rbuffer->pclock);
    }
    else if( rbuffer->Msg==REP)
    {printf("MHS - msg recieved REP with clock %d\n",rbuffer->pclock);
      REP_count++;
      
    
    
    }
    else if(rbuffer->Msg==REL)
    {printf("MHS - msg recieved REL with clock %d\n",rbuffer->pclock);
     dequeue();
       // pop of the top  of queue and signal if this process is in the top of the queue 
      
    
    }
    pthread_mutex_unlock(&critical);
 }

 void initChannelDelay(){  
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


   printf("Enter Total Number Of Processes (< 100):\n");  
   scanf("%d",&totalProcesses);  
   printf("Enter Process Id of Current Process (should be 0 to %d)\n",totalProcesses-1);  
   scanf("%d",&processId);  
   for(int p=0;p<totalProcesses;p++){  
     printf("Enter (Procees-%d) Port To Recevice Brodacast Message\n",p);  
     scanf("%d",&ports[p]);  
   }  
   
   sbuffer.pid = processId; 

   printf("This Procees Port No Is : %d\n",ports[processId]);  
   initChannelDelay();  
  
   pthread_t t;  

   if(pthread_create(&t, NULL, recvSES, NULL)){  
     DieWithError("Failed To Create Thread To Receive Brodcast Messages");  
   }  
   char str[100]; 
   int y;
   int src,dest; 
  


   for(;;){
      y=0;  
     printf("\nPress 1 for this process to execute critical section\n");  
     scanf("%d" ,&y);  
     if(y)
     Entry_section();

 }
}

 