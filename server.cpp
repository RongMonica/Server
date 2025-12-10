#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <iostream>
#include <unordered_map>
#include <random>
#include <array>
#include <cerrno>

using namespace std;

#define PORT 19879
#define BUFFER_SIZE 1024
time_t start_time;

//server receive command starts and ends with letters
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

//commmands that the test server receive. This function returns server's response to commands sent by client
string message_decoding(const string &s){
    static const unordered_map<string, char> message_decode = {
    {"time", 'A'},
    {"pid", 'B'},
    {"rand", 'C'},
    {"uptime", 'D'},
    {"echo hi", 'E'},
    {"exit", 'F'},
    {"hello", 'G'},};
    string reply;
    const time_t now = time(nullptr);
    auto it = message_decode.find(s);
    if(it == message_decode.end()){
        return "unknown command";
    }
    const char message_code = it->second;
    switch(message_code){
        case 'A':
            reply = string(ctime(&now));
            break;
        case 'B':
            reply = to_string(getpid());
            break;
        case 'C':
            reply = to_string(rand());
            break;
        case 'D':
            reply = to_string(now - start_time); //uptime in seconds
            break;
        case 'E':
            reply = s.substr(5);
            break;
        case 'F':
            reply = "bye";   
            break;
        case 'G':
            reply = "hi client";
            break;
        default:
            reply = "unknown command";
            break;
    }
    return reply;
}

//the server send reply after getting decoded messages
void response(int server_fd){
    ssize_t valread;
    array<char, BUFFER_SIZE> buffer;
    array<int, FD_SETSIZE> client_fds{};
    client_fds.fill(-1);

    fd_set readfds;
    int max_fd = server_fd;
    while(1){
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        for(int fd : client_fds){
            if(fd >= 0){
                FD_SET(fd, &readfds);
                if(fd > max_fd){
                    max_fd = fd;
                }
            }
        }

        if (select(max_fd + 1, &readfds, nullptr, nullptr, nullptr) < 0){
            perror("select");
            continue; 
        };

        // check for new incoming connection on the listening socket
        if (FD_ISSET(server_fd, &readfds)){
            int connfd;
            struct sockaddr_in address;
            socklen_t addrlen = sizeof(address);
            if ((connfd = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
                perror("Accept failed");
                continue;
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
                valread = read(fd, buffer.data(), buffer.size() - 1);
                if (valread <=0){
                    if(valread < 0){
                        perror("Read failed");
                    }
                    cout << "Client fd = " << fd << "disconnected" << endl;
                    close(fd);
                    client_fds[i] = -1;
                    continue;
                } else {
                    buffer[valread] = '\0';
                    cout << "Message from client: " << buffer.data() << endl;
                    string buffer_str(buffer.data(), static_cast<size_t>(valread));
                    string str_cmd = trim(buffer_str);
                    string reply = message_decoding(str_cmd);
                    ssize_t valsent = send(fd, reply.data(), reply.size(), 0);
                    if (valsent < 0) {
                        perror("Send failed");
                    } else {
                        cout << "Reply sent to client fd = " << fd << endl;
                    }
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
    start_time = time(nullptr);
    srand(time(nullptr));
    int server_fd;
    int backlog = 10; // max number of pending connection in queue

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // socket options (reuse address/port to allow quickly restart the server)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in address;
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
    
    cout << "Server running on port " << PORT << endl;
    response(server_fd);

    close(server_fd);
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
