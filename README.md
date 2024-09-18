# Socket-Send-Data-Multi-Process-Producer-Consumer
The project is based on the producer-consumer model. Throughout the project, I employed C++ to implement it. The client and server exchanged data through socket. In the server, tasks were put into the message queue and multi-process were used to process these tasks. I conducted this exercise to enhance understanding of these concepts and their practical applications. 

# Description
## Client:
Collect all file paths modified after the timestamp in the test folder, and send these file paths to the server.
You need to enter the timestamp. 
Server IP is set to 127.0.0.1 and port is set to 5005.
## Server:
Create two message queues `msgidFilePath` and `msgidChildResult`.

The father process creates multiple child processes. These child processes retrieve the file path from the message queue `msgidFilePath`, calculate the word frequency, store it in a hash table, and put the results into the message queue `msgidChildResult`.

The father process receives the file path sent by the client and puts it in the message queue `msgidFilePath`.

The father process gets the word frequency calculation results of all child processes from the message queue `msgidChildResult` and merge them in a hash table. Finally, print the hash table.

You need to enter the number of child processes.

# How to use
Compile:

`$ make`

Client command:

`$ ./client [Timestamp]`

![image](https://github.com/oh6400/Socket-Send-Data-Multi-Process-Producer-Consumer/blob/master/img/client.png)


Server command:

`$ ./server [Number of Processes]`

![image](https://github.com/oh6400/Socket-Send-Data-Multi-Process-Producer-Consumer/blob/master/img/server.png)
