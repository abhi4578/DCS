
# SES Protocol simulation implemented using socket programming in C language
  Can be implemented using mpi also

# Requirements
- OS : Ubuntu 16/18 (might work in all linux flavours, but haven't tested)
 

# Compile 
gcc ses.c -lpthread

# To run
- Run the executable file  in as much terminals as you want number of processes to be created
- Enter process id of that process(0-n-1)  in the terminal 
- Enter same set of port numbers for each process to bind  its port and connect to  other terminals
- Eg: If number of processes is 3
 - process 0 port 8050
 - process 1 port 9050 
 - process 2 port 9150
- enter the same set of port numbers in each terminal 
- Enter the message you want to send and the destination process id 

# To stop
-press ctrl+c(interrupt key to stop)

