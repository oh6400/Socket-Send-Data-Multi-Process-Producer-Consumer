#include <unordered_map>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <cstring>
#include <netdb.h>
#include <fstream>
#include <string>
#include <signal.h>
#include "public.h"

#define RECV_FILEPATH_MSG_TYPE 1
#define CHILD_RESULT_MSG_TYPE 2

struct Message {
    long msg_type;
    char data[FILE_PATH_LENGTH];
};

struct MessageFreq {
    long msg_type;
    char data[FILE_PATH_LENGTH];
    int cnt;
};

void mergeWordFrequency(const int &msgid, unordered_map<string, int> &mergeWordFreq, int numberOfProcess);

int calculateFrequency(const int &msgidFilePath, const int &msgidChildResult);

void SplitReceivedString(string recvBuf, const int &msgid);

int receiveFilePath(const int &msgid, int numberOfProcess);

int main(int argc, char *argv[]){
    if(argc != 2){
        cout << "Incorrect number of arguments.\n";
        cout << "Parameters: [number of process] ex: 8\n";
        return -1;
    }
    int numberOfProcess = atoi(argv[1]);

    // Create message queue for child process to calculate frequcecy
    key_t keyFilePath = ftok("filePath", 65);
    int msgidFilePath = msgget(keyFilePath, 0666 | IPC_CREAT);
    // Create message queue for merge frequcecy
    key_t keyChildResult = ftok("childResult", 65);
    int msgidChildResult = msgget(keyChildResult, 0666 | IPC_CREAT);

    signal(SIGCHLD, SIG_IGN);

    for(int i = 0; i < numberOfProcess; i++){
        pid_t pid = fork();
        if(pid == -1){
            perror("fork");
            return 1;
        }

        if(pid > 0) continue;

        calculateFrequency(msgidFilePath, msgidChildResult);
        return 0;
    }

    if(receiveFilePath(msgidFilePath, numberOfProcess) == -1) return -1;

    unordered_map<string, int> mergeWordFreq;
    mergeWordFrequency(msgidChildResult, mergeWordFreq, numberOfProcess);

    // Print words frequency
    for(auto w: mergeWordFreq){
        cout << w.first << ": " << w.second << endl;
    }

    msgctl(msgidFilePath, IPC_RMID, nullptr);
    msgctl(msgidChildResult, IPC_RMID, nullptr);
    printf("\n%s success.\n", __FILE__);

    return 0;
}

// Merge the results of all child processes
void mergeWordFrequency(const int &msgid, unordered_map<string, int> &mergeWordFreq, int numberOfProcess){
    int count = 0; // The number of child process completed
    MessageFreq msg;
    while(count < numberOfProcess){
        msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), CHILD_RESULT_MSG_TYPE, 0);
        if(msg.data[0] == '0'){ // There is a child process completed
            count++;
            continue;
        }
        mergeWordFreq[msg.data] += msg.cnt;
    }
}


// Generate several processes and Processing task
int calculateFrequency(const int &msgidFilePath, const int &msgidChildResult){
    // Record words frequency for this process
    unordered_map<string, int> wordFreq;
    Message msgRecv;
    msgRecv.msg_type = RECV_FILEPATH_MSG_TYPE;
    while (1)
    {
        // Get a task from queue
        msgrcv(msgidFilePath, &msgRecv, sizeof(msgRecv) - sizeof(long), 1, 0);
        if(msgRecv.data[0] == '0') break;

        // Processing task: Calculate words frequency for this file
        std::ifstream file;
        file.open(msgRecv.data);
        string line, word;
        while(std::getline(file, line)){
            stringstream ss(line);
            while(ss >> word){
                if(ispunct(word[word.length()-1])) // Remove punctuation
                    word = word.substr(0, word.length()-1);
                wordFreq[word]++;
            }
        }
        file.close();
    }

    // Return the results to the parent process for merge
    MessageFreq msg;
    msg.msg_type = CHILD_RESULT_MSG_TYPE;
    for(auto w: wordFreq){
        strcpy(msg.data, w.first.c_str());
        msg.cnt = w.second;
        msgsnd(msgidChildResult, &msg, sizeof(msg) - sizeof(long), 0);
    }

    // Mark this process complete
    char endFlag[] = "0";
    strcpy(msg.data, endFlag);
    msg.cnt = 0;
    msgsnd(msgidChildResult, &msg, sizeof(msg) - sizeof(long), 0);
    
    return 0;
}


// Split the received string with '\n' and notify one of the child process.
void SplitReceivedString(string recvBuf, const int &msgid){
    std::stringstream ss(recvBuf);
    string filePath;
    while(std::getline(ss, filePath, '\n')){
        if(filePath.length() > 0){
            Message msg;
            msg.msg_type = RECV_FILEPATH_MSG_TYPE;
            strcpy(msg.data, filePath.c_str());
            msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0);
        }
    }
}

// Receive file paths from clinet
int receiveFilePath(const int &msgid, int numberOfProcess){
    // Create server socket 
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd == -1){
        cerr << "Fail to create server socket\n";
        return -1;
    }

    // Bind IP to socket
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(INPUT_PORT);
    if (bind(listenFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Bind failed\n";
        close(listenFd);
        return -1;
    }

    // Set the socket to listenable
    // backlog = 3: Accepts at most 3 connection requests.
    if (listen(listenFd, 3) < 0) {
        cerr << "Listen failed\n";
        close(listenFd);
        return -1;
    }

    // Handle client connection requests
    int clientFd = accept(listenFd, 0, 0);
    if(clientFd == -1){
        cerr << "client connection requests failed\n";
        close(listenFd);
        return -1;
    }

    char recvBuf[FILE_PATH_LENGTH] = {0}; // message buffer
    string recvString; // receive message
    // Communicate with client
    // Receive messages and push into message queue.
    while(1){
        int iret = recv(clientFd, recvBuf, sizeof(recvBuf), 0);
        if(iret <= 0){
            // cout << "receive message from client failed.\n";
            break;
        }
        recvString = recvBuf;
        SplitReceivedString(recvString, msgid);
        memset(recvBuf, 0, sizeof(recvBuf));
        string reply = "OK";
        iret = send(clientFd, reply.c_str(), reply.size(), 0);
        if(iret <= 0){
            cout << "send message to client failed.\n";
            break;
        }
    }
    
    // Close socket
    close(listenFd);
    close(clientFd);    
    
    // Mark no more task
    for(int i = 0; i < numberOfProcess; i++){
        Message msg;
        msg.msg_type = RECV_FILEPATH_MSG_TYPE;
        char endFlag[] = "0";
        strcpy(msg.data, endFlag);
        msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0);
    }

    return 0;
}
