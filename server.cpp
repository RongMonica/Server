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

void response(int server_fd){
    ssize_t valread;
    char buffer[1024] = { 0 };
    int client_fds[FD_SETSIZE];
    for(int i = 0; i < FD_SETSIZE; i++){
        client_fds[i] = -1;
    }

    fd_set readfds;
    int max_fd = server_fd;
    while(1){
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        for(int i = 0; i < max_fd + 1; i++){
            if(client_fds[i] >= 0){
                FD_SET(client_fds[i], &readfds);
                if(client_fds[i] > max_fd){
                    max_fd = client_fds[i];
                }
            }
        }

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0){
            perror("select");
            exit(EXIT_FAILURE); 
        };

        // check for new incoming connection on the listening socket
        if (FD_ISSET(server_fd, &readfds)){
            int connfd;
            struct sockaddr_in address;
            socklen_t addrlen = sizeof(address);
            if ((connfd = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            } else {
                int stored = 0;
                for (int i = 0; i < FD_SETSIZE; i++){
                    if (client_fds[i] == -1){
                        client_fds[i] = connfd;
                        max_fd = max(max_fd, connfd);
                        FD_SET(connfd, &readfds);
                        stored = 1;
                        cout << "New connection, fd = " << connfd << " at slot: " << i << endl;
                        break;
                    }
                }
                if (!stored){
                    cout << "Too many connections, rejecting fd = " << connfd << endl;
                    close(connfd);
                }
            }
        }

        // check existing clients for incoming data
        for (int i = 0; i < max_fd + 1; i++){
            int fd = client_fds[i];
            if (fd == -1) continue;
            if (FD_ISSET(fd, &readfds)){
                valread = read(fd, buffer, sizeof(buffer) - 1);
                if (valread <=0){
                    if(valread < 0){
                        perror("read");
                    }
                    cout << "Client fd = " << fd << "disconnected" << endl;
                    close(fd);
                    client_fds[i] = -1;
                } else {
                    buffer[valread] = '\0';
                    cout << "Message from client: " << buffer << endl;
                    string buffer_str(buffer);
                    buffer_str = trim(buffer_str);
                    string reply = message_decoding(buffer_str);
                    const char *reply_c = reply.c_str();
                    send(fd, reply_c, reply.size(), 0);
                    cout << "Reply sent to client fd = " << fd << endl;
                    if (reply == "bye"){
                        close(fd);
                        client_fds[i] = -1;
                    }
                }
            }
        }
    }
}

int main(int argc, char const* argv[])
{
    start_time = time(NULL);
    int server_fd;
    int backlog = 10; // max number of pending connection in queue
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // socket options (reuse address/port to allow quickly restart the server)
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address))< 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, backlog) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    response(server_fd);

    return 0;
}

/*test message you could send to a server: 
hello: server replies "hi client"
time: get current server time
pid: get process ID
rand: server sends a random number
uptime: server returns how long it has been running
echo hi: server replies hi
exit: client closes connection gracefully. The server reply with "bye"
*/ 
