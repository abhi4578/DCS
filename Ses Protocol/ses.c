#include <stdio.h>  
 #include <sys/socket.h>  
 #include <arpa/inet.h>  
 #include <stdlib.h>  
 #include <string.h>  
 #include <unistd.h>  
 #include <pthread.h> 
 #include<semaphore.h> 

 #define MAXPENDING 10  



 int totalProcesses;  
 int processId;  
 unsigned int ports[10];  
 int proccessId;  
  sem_t mutex;
 int channelDelay[10];  
 int delivBuf[100][100];  
 int delivBufIndex=0;  
 
  struct VP                  
 {
  int clock[10];
 };

typedef  struct info {
      int del;
     int pclock[10];
     struct VP Vp[10]; /*VectorM associated with each process-Contains the history of communiction*/
     char  Msg[1000];
 }buffer;
 
typedef struct 
 { int sock;
    struct sockaddr_in peer;   
 }arguments;

 struct node {  
   buffer rbuffer;
   int isDelivered;  
   struct node *next;  
 };  

 struct node *head_Ses;  
 void Ack(buffer rbuffer);
int Merge(buffer *rbuffer);
int checkVp(buffer *rbuffer);
int tm_lessthan_tp(int clock[],int pclock[]);
void intialise();
buffer sbuffer;
 void DieWithError(char *errorMessage){  
   perror (errorMessage) ;  
   exit(1);  
 }  
 void addToBuffer(buffer *rbuffer){  
   struct node *temp = (struct node*) malloc(sizeof(struct node));  
     
   temp->rbuffer=*rbuffer;  
   temp->isDelivered=0;
   temp->next = head_Ses;  
   head_Ses = temp;  
   free(rbuffer);
 }  
 void displayLocalClock(){  
   printf("Local Clock \n");  
   for(int p =0;p<totalProcesses;p++){  
     printf("%d,",sbuffer.pclock[p]);  
   }  
    printf("\n\n");  
 }  
 void displayBuffer(){  
   struct node* temp =head_Ses;  
   printf("Buffered Messages Clock \n");  
   while(temp !=NULL){  
     for(int p =-1;p<totalProcesses;p++){  
       printf("-");  
     }  
     printf("\n");  
     if(!temp->isDelivered)
    {  printf("Tm of message: ");
     Ack(temp->rbuffer) ;
   }
     printf("\n");  
     for(int p =-1;p<totalProcesses;p++){  
       printf("-");  
     }  
     temp = temp->next;  
   }  
   printf("\n\n");  
 }  
 void checkBufferAndDeliver(struct sockaddr_in peerAddr){  
   struct node* temp =head_Ses;  
   while(temp !=NULL && !(temp->isDelivered)){  
     
       if(tm_lessthan_tp(temp->rbuffer.Vp[processId].clock,sbuffer.pclock)) {
         printf("\nDelivered Message From:- %s(%d), proc no.:- %d \nMsg: %s\n", inet_ntoa(peerAddr.sin_addr), ports[temp->rbuffer.pclock[totalProcesses]], temp->rbuffer.pclock[totalProcesses],temp->rbuffer.Msg); 
        
         temp->isDelivered =1;  
         sbuffer.pclock[processId]++;
         for(int c=0;c<totalProcesses;c++){  
           sbuffer.pclock[c] = sbuffer.pclock[c] > temp->rbuffer.pclock[c] ? sbuffer.pclock[c] : temp->rbuffer.pclock[c];  

         }  
      
          Merge(&(temp->rbuffer));
           //displayLocalClock();  
       printf("Message recieved with tm: ");
       Ack(sbuffer);
        }
     temp = temp->next;  
     }
 }  
 void handleDelivery(buffer *rbuffer,struct sockaddr_in peerAddr){  
   // Check all messages are recived which sent from sender before this event  
    sbuffer.pclock[processId]++;
     int deliver=checkVp(rbuffer);
     if(deliver){  
       printf("\nDelivered Message From:- %s(%d), proc no.:- %d \nMsg: %s\n", inet_ntoa(peerAddr.sin_addr), ports[rbuffer->pclock[totalProcesses]], rbuffer->pclock[totalProcesses],rbuffer->Msg);  
       
       for(int c=0;c<totalProcesses;c++){  
         sbuffer.pclock[c] =sbuffer.pclock[c] > rbuffer->pclock[c] ? sbuffer.pclock[c] : rbuffer->pclock[c];  
       } 
       
       Merge(rbuffer);
       //displayLocalClock();  
       printf("Message recieved with tm: ");
       Ack(sbuffer);
       checkBufferAndDeliver(peerAddr);  
     }
     else if(tm_lessthan_tp(rbuffer->Vp[processId].clock,sbuffer.pclock))
     {  
       printf("\nDelivered Message From:- %s(%d), proc no.:- %d \nMsg: %s\n", inet_ntoa(peerAddr.sin_addr), ports[rbuffer->pclock[totalProcesses]], rbuffer->pclock[totalProcesses],rbuffer->Msg); 
       //sbuffer.pclock[processId]++;
       for(int c=0;c<totalProcesses;c++){  
         sbuffer.pclock[c] = sbuffer.pclock[c] > rbuffer->pclock[c] ? sbuffer.pclock[c] : rbuffer->pclock[c];  
       }  
       Merge(rbuffer);
        //displayLocalClock();  
       printf("Message recieved with tm: ");
       Ack(sbuffer);
       checkBufferAndDeliver(peerAddr);  
     }  
   else{ 
    sbuffer.pclock[processId]--; 
     addToBuffer(rbuffer);  
     printf("*** All message sent from %s(%d) before this event is not yet received. Putting into buffer *****\n",inet_ntoa(peerAddr.sin_addr), ports[rbuffer->pclock[totalProcesses]]);  
     displayLocalClock();  
     displayBuffer();  
   }  
 } 

 int Merge(buffer *rbuffer)
 {
  int i;
  for(i=0;i<totalProcesses;i++)
  { 
    if(i==processId)
    {
      
    
   /* if(tm_lessthan_tp(sbuffer.Vp[i].clock,sbuffer.pclock) )
    {
      for(int j=0;j<totalProcesses;j++)
      sbuffer.Vp[i].clock[j]=0;
    }*/
    continue;
  }
    else
    { for(int j=0;j<totalProcesses;j++)
      sbuffer.Vp[i].clock[j]=sbuffer.Vp[i].clock[j] > rbuffer->Vp[i].clock[j] ? sbuffer.Vp[i].clock[j] : rbuffer->Vp[i].clock[j];
    }

  }

 }
 int checkVp(buffer *rbuffer)
 {
  for(int i=0;i<totalProcesses;i++)
    if(rbuffer->Vp[processId].clock[i]!=0)
      return 0;
  return 1;
 }

int tm_lessthan_tp(int clock[],int pclock[])
{int flag=0;
 
  for(int i=0;i<totalProcesses;i++)
  {if(clock[i]<pclock[i])
      flag=1;
  else if(clock[i]>pclock[i])
    
    { 
      return 0;}
    }
    
    return flag;
}
 void *handleTCPClient(void *args1){ 
    arguments *args=args1; 
   int clntSocket=args->sock;
   struct sockaddr_in peerAddr=args->peer;
   free(args);
   buffer *rbuffer=(buffer *)malloc(sizeof(buffer));
   int recvMsgSize;  
   if ((recvMsgSize = recv(clntSocket, rbuffer, sizeof(buffer), 0)) < 0)  
     DieWithError("recv() failed");  
   sleep(rbuffer->del);  
   printf("\nReceived Message From:- %s(%d), proc no. :- %d \n", inet_ntoa(peerAddr.sin_addr), ports[rbuffer->pclock[totalProcesses]], rbuffer->pclock[totalProcesses]);  
   sem_wait(&mutex);
   handleDelivery(rbuffer,peerAddr);  //critical section
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
 void sendSES(char * msgsend,int p){  
   int sendSock; 
   
   struct sockaddr_in peerAddr;  
   char *peerlP = "127.0.0.1";  
   sbuffer.pclock[processId]++; 
   sbuffer.del=channelDelay[p% totalProcesses];
   strcpy(sbuffer.Msg,msgsend);
   int sent=0;  
   
     if ((sendSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)  
       DieWithError("socket () failed\n") ;  
     memset(&peerAddr, 0, sizeof(peerAddr));  
     peerAddr.sin_family = AF_INET;  
     peerAddr.sin_addr.s_addr = inet_addr(peerlP);  
     peerAddr.sin_port = htons(ports[ p % totalProcesses]);  
     
     if (connect(sendSock, (struct sockaddr *) &peerAddr, sizeof(peerAddr)) < 0)  
       DieWithError("connect () failed\n");  
     if (send(sendSock, &sbuffer, sizeof(sbuffer), 0) != sizeof(buffer))  
       DieWithError("send() sent a different number of bytes than expected\n");  
     printf("message sent with tm: ");
     Ack(sbuffer);
     for(int i=0;i<totalProcesses;i++)
      sbuffer.Vp[p].clock[i]=sbuffer.pclock[i];
   
 }  
 void initChannelDelay(){  
   int delay=2;  
   channelDelay[processId] = 0;  
   int chupdated=1;  
   for(int ch=processId+1;chupdated<totalProcesses;ch++){  
     channelDelay[ ch % totalProcesses ] = delay;  
     delay+=15;  
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

   printf("Enter Total Number Of Processes (< 10):\n");  
   scanf("%d",&totalProcesses);  
   printf("Enter Process Id of Current Process (should be 0 to %d)\n",totalProcesses-1);  
   scanf("%d",&processId);  
   for(int p=0;p<totalProcesses;p++){  
     printf("Enter (Procees-%d) Port To Recevice Brodacast Message\n",p);  
     scanf("%d",&ports[p]);  
   }  
   intialise();
   sbuffer.pclock[totalProcesses] = processId;  
   printf("This Procees Port No Is : %d\n",ports[processId]);  
   initChannelDelay();  
   pthread_t t;  
   if(pthread_create(&t, NULL, recvSES, NULL)){  
     DieWithError("Failed To Create Thread To Receive Brodcast Messages");  
   }  
   char str[100]; 
   int src,dest; 
   for(;;){  
     printf("\nEnter something to send  message\n");  
     scanf("%s" ,str);  
     printf("Enter dest\n");  
     scanf("%d",&dest);
     sendSES(str,dest);

 }
}

 void intialise()
 {
 	int i,j;
 	for(i=0;i<totalProcesses;i++)
 	{
 		sbuffer.pclock[i]=0;
 	}
 	for(i=0;i<totalProcesses;i++)
 		for(j=0;j<totalProcesses;j++)
 			sbuffer.Vp[i].clock[j]=0;
 }


 void Ack(buffer  rbuffer)
 {for(int i=0;i<totalProcesses;i++)
      printf("%d,",rbuffer.pclock[i]);
      printf("\t Vp:");
      for(int i=0;i<totalProcesses;i++)
        { printf("(%d,(",i);
      for(int j=0;j<totalProcesses;j++)
     { printf("%d,",rbuffer.Vp[i].clock[j]);
      }
      printf(")) ");}

 }