Simple Multi-Client TCP Server

This is a simple TCP server written in C++.
It listens on port 8080, accepts multiple clients using select(), and responds to predefined text commands.

The server is intended for learning how to use:
socket, bind, listen, accept
select() for multiplexing multiple clients
basic string parsing and simple command protocol

Build
g++ server.cpp -o server

Run
Start the server:
./server
You should see log messages when clients connect:
New connection, fd = 5 at slot: 0

Behavior
Creates a listening socket on port 8080
Sets SO_REUSEADDR | SO_REUSEPORT to allow fast restarts
Uses select() to monitor:
the listening socket (new incoming connections)
all active client sockets
Stores client file descriptors in an array (size = FD_SETSIZE)

For each client:
reads incoming data
trims the command
decodes the command
sends back the server reply
Disconnected clients are automatically cleaned up.
The server runs indefinitely until manually stopped (Ctrl+C).

Supported Commands
These text commands can be sent by any connected client:

Command	Description / Server Reply
hello	hi client
time	Current server time (ctime())
pid	Process ID of the server
rand	Random number (rand())
uptime	Seconds since the server started
echo hi	Replies hi (everything after echo )
exit	Sends bye and closes that client’s socket

If a command is unknown, the server returns:
unknown command

Multi-Client Handling (select)
The server supports multiple simultaneous clients
No threads or fork() are used

select() monitors:
listening socket for new connections
all connected client sockets for data
Max clients is limited by FD_SETSIZE (usually 1024)

If the server runs out of slots:
Too many connections, rejecting fd = <n>

Output Example
When a client sends:
hello

Server logs:
Message from client: hello
Reply sent to client fd = 5

And the client receives:
hi client

Exit Conditions
A client connection is closed when:
The client sends exit
The client disconnects (read() returns 0)
A socket error occurs
The server itself only stops when you press Ctrl+C.

Notes
The server expects simple text commands separated by newlines.
This project is ideal for learning:
basic TCP networking
handling multiple clients
simple text-based protocol design
