#include<stdio.h>
#include<string.h>    
#include<stdlib.h>    
#include<sys/socket.h>
#include<arpa/inet.h> 
#include<unistd.h>    
#include<thread> 
#include<vector>
#include<mutex>

typedef std::lock_guard<std::mutex> glock;

/* 
 * Simple tcp chat server using posix sockets 
 * binds to port 8888, uses c++11 threads and mutex
 *
 * Usage: start chat server './cserver' in one terminal
 * open two or more terminals and connect via telnet
 * 'telnet localhost 8888'.  (escape ctrl],quit)
 * Messages from one client will be sent to the other chat clients
 *
 * compile:
 * g++ accept.cpp -o cserver -std=c++11 -pthread -g
 */


/*
 * clientMgr - provides interface to add new client sockets and send msgs to all clients
 * while protecting the client socket descriptor list from multiple thread access;
 * */

class ClientMgr {
  std::vector<int> clientFDs;
  std::mutex ClientList;

public:
  void sendMsgToClients(int sock, char *msg) {
    glock guard( ClientList );
    
    //Distribue to other clients except yourself
    for(auto &s : clientFDs) {
      if(s== sock) continue;
          write(s , msg , strlen(msg));          
    }
  }

  void addClient(int clientFd){
    glock guard( ClientList );
    puts("Connection accepted");
    clientFDs.push_back(clientFd);
  }
};


/*
 * Connection handler for each client, waits for a msg from the client and distrbutes. 
 * */

void *connection_handler(void *socket_desc,int i, ClientMgr *clientMgr)
{
    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[2000];    
    char message[100];
         
    snprintf( message, 100, "Connection handler No:%d\nStart Chat session .... \n", i); 
    write(sock , message , strlen(message));   
    
   //Main loop - Receive a message from client and distribute to others
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
    {
        clientMgr->sendMsgToClients(sock, client_message);
    }
     
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
         
    //Free the socket pointer and close connection
    shutdown(*(int*)socket_desc,SHUT_RDWR);
    close(*(int*)socket_desc);
    free(socket_desc);
     
    return 0;
}

class ChatServer {

public:
    std::vector<std::thread> threads; 
    int socket_desc;    
    struct sockaddr_in server;
    ClientMgr *cmgr;
    
    ChatServer(ClientMgr *cm): cmgr(cm){
      initConnection();
    }

    void initConnection(){
      
      //Create socket
      socket_desc = socket(AF_INET , SOCK_STREAM , 0);
      if (socket_desc == -1) {
        printf("Could not create socket");
      }
    
      /* Set SO_REUSEADDR option active */
      int optval = 1;
      socklen_t optlen = sizeof(optval);
      if(setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(socket_desc);
        exit(EXIT_FAILURE);
      }

 
      //Prepare the sockaddr_in structure
      server.sin_family = AF_INET;
      server.sin_addr.s_addr = INADDR_ANY;
      server.sin_port = htons( 8888 );
     
      //Bind
      if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
      {
        puts("bind failed");        
      }
      puts("bind done");
      
        //Listen
      listen(socket_desc , 3);      
    }

    void acceptConns(){
      
      int new_socket, *new_sock;
      struct sockaddr client;
      char const *message;
      int i;
            
      puts("Waiting for incoming connections...");
      int cnt = sizeof(struct sockaddr_in);
      while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&cnt)) ) {
        cmgr->addClient(new_socket);
        message = "Received your connection assigning a handler for you\n";
        write(new_socket , message , strlen(message));         
        
        new_sock = (int*)malloc(1);
        *new_sock = new_socket;
        i=threads.size();
        threads.emplace_back(std::thread( connection_handler, (void*) new_sock, i,cmgr));  
        puts("Handler assigned");
      }
     
      if (new_socket<0) {
        perror("accept failed");        
      }      
    } 

    ~ChatServer() {
      for(auto &x : threads)
        if(x.joinable())
          x.join();
    } 

};

 
int main(int argc , char *argv[])
{
  ClientMgr cm;
  ChatServer cs(&cm);
  cs.acceptConns();
}
 


