#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include <string>
#include <iostream>
#include <unordered_map>
#include <random>
using namespace std;

#define PORT 8080
time_t start_time;

string trim(const string &str){
    string str_cmd;
    int start, end;
    for(int i = 0; i <= end; i++){
        if(isalpha(str[i])){
            start = i;
            break;
        }
    }
    for(int j = str.size()-1; j >=0; j--){
        if(isalpha(str[j])){
            end = j;
            break;
        }
    }
    str_cmd = str.substr(start, end - start + 1);
    return str_cmd;
}

string message_decoding(const string &s){
    unordered_map<string, int> message_code = {
    {"time", 1},
    {"pid", 2},
    {"rand", 3},
    {"uptime", 4},
    {"echo hi", 5},
    {"exit", 6},
    {"hello", 7},};
    string reply;
    time_t now = time(nullptr);
    int message_decode = message_code[s];
    switch(message_decode){
        case 1:
            reply = ctime(&now);
            break;
        case 2:
            reply = to_string(getpid());
            break;
        case 3:
            reply = to_string(rand());
            break;
        case 4:
            reply = to_string(now - start_time);
            break;
        case 5:
            reply = s.substr(5);
            cout << reply << endl;
            break;
        case 6:
            reply = "bye";   
            break;
        case 7:
            reply = "hi client";
            break;
        default:
            reply = "unknown command";
            break;
    }
    return reply;
}

void *response(void* input){
    int new_socket = (int)(intptr_t)input;
    ssize_t valread;
    char buffer[1024] = { 0 };
    char reply[1024];
    
    while(1){
        valread = read(new_socket, buffer, sizeof(buffer) - 1); // subtract 1 for the null
        if(valread <=0) break;
        buffer[valread] = '\0';
        cout << "Client said: " << buffer << endl;

        string buffer_str(buffer);
        buffer_str = trim(buffer_str);
        string reply = message_decoding(buffer_str);
        const char *reply_c = reply.c_str();
        if(reply == "bye"){
            send(new_socket, reply_c, reply.size(), 0);
            close(new_socket);
        }else{
            send(new_socket, reply_c, reply.size(), 0);
        }
        
        printf("Response sent\n");
    }
    close(new_socket);
    return nullptr;
}

int main(int argc, char const* argv[])
{
    start_time = time(NULL);
    int server_fd, new_socket;
    int backlog = 3; // max number of pending connection in queue
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);


    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // socket options (reuse address/port to allow quickly restart the server)
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address))< 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, backlog) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while(1){
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        pthread_t tid;
        pthread_create(&tid, NULL, response, (void *)(intptr_t)new_socket);
    }
  
    // closing the listening socket
    close(server_fd);
    return 0;
}

/*message you could send to a server: 
hello: server replies "hi client"
time: get current server time
pid: get process ID
rand: server sends a random number
uptime: server returns how long it has been running
echo hi: server replies hi
exit: client closes connection gracefully. The server reply with "bye"
*/ 
