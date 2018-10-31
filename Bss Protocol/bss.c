#include <stdio.h>  
 #include <sys/socket.h>  
 #include <arpa/inet.h>  
 #include <stdlib.h>  
 #include <string.h>  
 #include <unistd.h>  
 #include <pthread.h>  
 #define MAXPENDING 10  
 int totalProcesses;  
 int processId;  
 unsigned int ports[10];  
 int proccessId;  
 int pclock[10];  
 int channelDelay[10];  
 int delivBuf[100][100];  
 int delivBufIndex=0;  
 struct node {  
   int rclock[10];  
   int isDelivered;  
   struct node *next;  
 };  
 struct node *head;  
 void DieWithError(char *errorMessage){  
   perror (errorMessage) ;  
   exit(1);  
 }  
 void addToBuffer(int *rclock){  
   struct node *temp = (struct node*) malloc(sizeof(struct node));  
   for(int p=0;p<=totalProcesses;p++){  
     temp->rclock[p] = *(rclock+p);  
   }  
   temp->isDelivered =0;  
   temp->next = head;  
   head = temp;  
 }  
 void displayLocalClock(){  
   printf("Local Clock \n");  
   for(int p =0;p<totalProcesses;p++){  
     printf("%d,",pclock[p]);  
   }  
    printf("\n\n");  
 }  
 void displayBuffer(){  
   struct node* temp =head;  
   printf("Buffered Messages Clock \n");  
   while(temp !=NULL){  
     for(int p =-1;p<totalProcesses;p++){  
       printf("-");  
     }  
     printf("\n");  
     for(int p =0;p<totalProcesses;p++){  
       printf("%d|",temp->rclock[p]);  
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
   struct node* temp =head;  
   while(temp !=NULL && !(temp->isDelivered)){  
     if(pclock[temp->rclock[totalProcesses]] == temp->rclock[temp->rclock[totalProcesses]] -1){  
       int deliver =1;  
       for(int c=0;c<totalProcesses;c++){  
         if(c!= temp->rclock[totalProcesses]){  
           if(pclock[c] < temp->rclock[c]){  
             deliver=0;  
             break;  
           }  
         }  
       }  
       if(deliver){  
         printf("\nDelivered BUffered Message Which Was Received From:- %s(%d), Msg :- %d \n", inet_ntoa(peerAddr.sin_addr), ports[temp->rclock[totalProcesses]], temp->rclock[temp->rclock[totalProcesses]]);  
         temp->isDelivered =1;  
         for(int c=0;c<totalProcesses;c++){  
           pclock[c] = pclock[c] > temp->rclock[c] ? pclock[c] : temp->rclock[c];  
         }  
       }  
     }  
     temp = temp->next;  
   }  
 }  
 void handleDelivery(int *rclock,struct sockaddr_in peerAddr){  
   // Check all messages are recived which sent from sender before this event  
   if(pclock[*(rclock +totalProcesses)] == *(rclock +(*(rclock +totalProcesses))) -1){  
     int deliver =1;  
     for(int c=0;c<totalProcesses;c++){  
       if(c!= (*(rclock+totalProcesses))){  
         if(pclock[c] <(*(rclock+c))){  
           deliver=0;  
           break;  
         }  
       }  
     }  
     if(deliver){  
       printf("\nDelivered Message From:- %s(%d), Msg :- %d \n", inet_ntoa(peerAddr.sin_addr), ports[*(rclock+totalProcesses)], *(rclock+ (*(rclock+totalProcesses))));  
       for(int c=0;c<totalProcesses;c++){  
         pclock[c] = pclock[c] > (*(rclock+c)) ? pclock[c] : (*(rclock+c));  
       }  
       displayLocalClock();  
       checkBufferAndDeliver(peerAddr);  
     }else{  
       addToBuffer(rclock);  
       printf("*** All messages sent from other proceeses before this event is not yet received. Putting into buffer *****\n");  
       displayLocalClock();  
       displayBuffer();  
     }  
   }else{  
     addToBuffer(rclock);  
     printf("*** All message sent from %s(%d) before this event is not yet received. Putting into buffer *****\n",inet_ntoa(peerAddr.sin_addr), ports[(*(rclock+totalProcesses))]);  
     displayLocalClock();  
     displayBuffer();  
   }  
 }  
 void handleTCPClient(int clntSocket,struct sockaddr_in peerAddr){  
   int rclock[10];  
   int recvMsgSize;  
   if ((recvMsgSize = recv(clntSocket, &rclock, sizeof(rclock), 0)) < 0)  
     DieWithError("recv() failed");  
   printf("\nReceived Message From:- %s(%d), Msg :- %d \n", inet_ntoa(peerAddr.sin_addr), ports[rclock[totalProcesses]], rclock[rclock[totalProcesses]]);  
   handleDelivery(rclock,peerAddr);  
   close(clntSocket);  
 }  
 void *recvBCMsg(void *attribute){  
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
     DieWithError("listen() failed\n");  
   printf("\nSocket Binded To Recv Messages\n");  
   for (;;){  
     peerLen = sizeof(peerAddr);  
     if ((peerSock = accept(recvSock, (struct sockaddr *) &peerAddr,&peerLen)) < 0)  
       DieWithError("accept() failed\n");  
     handleTCPClient(peerSock,peerAddr);  
   }  
 }  
 void sendBCMsg(char * msgsend){  
   int sendSock;  
   struct sockaddr_in peerAddr;  
   char *peerlP = "127.0.0.1";  
   pclock[processId] = pclock[processId] + 1;  
   int sent=0;  
   for(int p=processId+1;sent<totalProcesses-1;p++){  
     if ((sendSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)  
       DieWithError("socket () failed\n") ;  
     memset(&peerAddr, 0, sizeof(peerAddr));  
     peerAddr.sin_family = AF_INET;  
     peerAddr.sin_addr.s_addr = inet_addr(peerlP);  
     peerAddr.sin_port = htons(ports[ p % totalProcesses]);  
     sleep(channelDelay[p % totalProcesses]);  
     if (connect(sendSock, (struct sockaddr *) &peerAddr, sizeof(peerAddr)) < 0)  
       DieWithError("connect () failed\n");  
     if (send(sendSock, &pclock, sizeof(pclock), 0) != sizeof(pclock))  
       DieWithError("send() sent a different number of bytes than expected\n");  
     sent++;  
   }  
 }  
 void initChannelDelay(){  
   int delay=2;  
   channelDelay[processId] = 0;  
   int chupdated=1;  
   for(int ch=processId+1;chupdated<totalProcesses;ch++){  
     channelDelay[ ch % totalProcesses ] = delay;  
     delay+=2;  
     chupdated++;  
   }  
   printf("Channel Delays to Process %d to %d ----> ",0,totalProcesses-1);  
   for(int ch =0 ;ch<totalProcesses;ch++){  
     printf("%d,",channelDelay[ch]);  
   }  
   printf("\n");  
 }  
 int main(int argc, const char * argv[])  
 {  
   printf("Enter Total Number Of Processes (< 10):\n");  
   scanf("%d",&totalProcesses);  
   printf("Enter Process Id of Current Process (should be 0 to %d)\n",totalProcesses-1);  
   scanf("%d",&processId);  
   for(int p=0;p<totalProcesses;p++){  
     printf("Enter (Procees-%d) Port To Recevice Brodacast Message\n",p);  
     scanf("%d",&ports[p]);  
   }  
   pclock[totalProcesses] = processId;  
   printf("This Procees Port No Is : %d\n",ports[processId]);  
   initChannelDelay();  
   pthread_t t;  
   if(pthread_create(&t, NULL, recvBCMsg, NULL)){  
     DieWithError("Failed To Create Thread To Receive Brodcast Messages");  
   }  
   char str[100];  
   for(;;){  
     printf("Enter something to send broadcast message\n");  
     scanf("%s" ,str);  
     sendBCMsg(str);  
   }  
 }  