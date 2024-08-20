#include "public.h"

#define INPUT_HOST "127.0.0.1"

void getModifiedFiles(const time_t& timestamp, vector<string>& filePaths);

int sendToServer(const string &files_str, int &socketFd);

void sendFilesToServer(vector<string>& filePaths);

int main(int argc, char *argv[]){
    if(argc != 2){
        cout << "Incorrect number of arguments.\n";
        cout << "Parameters: [Timestamp] ex: 1719837120\n";
        return -1;
    }
    time_t inputTimestamp = atoi(argv[1]);

    vector<string> filePaths;
    
    getModifiedFiles(inputTimestamp, filePaths);

    sendFilesToServer(filePaths);

    printf("%s success.\n", __FILE__);

    return 0;
}

void getModifiedFiles(const time_t& timestamp, vector<string>& filePaths){
    for(const auto& entry: fs::recursive_directory_iterator("../test1", fs::directory_options::skip_permission_denied)){
        if(fs::is_regular_file(entry)){
            auto ftime = fs::last_write_time(entry);
            std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
            if (cftime > timestamp) {
                filePaths.push_back(entry.path().string());
            }
        }
    }
}

int sendToServer(const string &files_str, int &socketFd){
    char recvBuf[1024] = {0};
    int iret = send(socketFd, files_str.c_str(), files_str.size(), 0);
    if(iret <= 0){
        cout << "send message to server failed.\n";
        return iret;
    }

    iret = recv(socketFd, recvBuf, sizeof(recvBuf), 0);
    if(iret <= 0){
        cout << "receive message from server failed\n";
        return iret;
    }
    
    return iret;
}

void sendFilesToServer(vector<string>& filePaths){
    // create client socket    
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketFd == -1){
        cerr << "Fail to create a client socket\n";
        return;
    }
        
    // Make a request to the server
    struct hostent* h = gethostbyname(INPUT_HOST);
    if(h == 0){
        cerr << "gethostbyname failed.\n";
        close(socketFd);
        return;
    }
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(INPUT_PORT);
    memcpy(&serverAddr.sin_addr, h->h_addr, h->h_length);
    if (connect(socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connection Failed\n";
        close(socketFd);
        return;
    }

    // Communicate with server
    std::string files_str = "";
    int currentLength = 0;
    for (const auto& file : filePaths) {
        if(currentLength + file.length() + 1 > 1024){
            if(sendToServer(files_str, socketFd) <= 0) break;
            files_str = "";
            currentLength = 0;
        }
        files_str += (file + "\n");
        currentLength += (file.length() + 1);
    }
    if(currentLength > 0) sendToServer(files_str, socketFd);

    // close socket
    close(socketFd);
}
