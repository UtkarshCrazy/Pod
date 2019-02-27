#include "UDPManager.hpp"

using namespace std;
using namespace Utils;

int UDPManager::socketfd = 0;
std::atomic<bool> UDPManager::running(false);
UDPManager::Connection_Status UDPManager::connection_status = UDPManager::Connection_Status::NOT_YET_CONNECTED;
struct addrinfo UDPManager::hints;
struct addrinfo * UDPManager::sendinfo = NULL;
struct addrinfo * UDPManager::recvinfo = NULL;


bool UDPManager::start_udp(const char * hostname, const char * send_port, const char * recv_port){
  int rv;
  int enable = 1;

  /////////////////////
  // SETUP UDP SEND PORT
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  
  // Get destination address
  if((rv = getaddrinfo(hostname, send_port, &hints, &sendinfo)) != 0) {
    print(LogLevel::LOG_ERROR, "UDP send Error get addrinfo: %s\n", gai_strerror(rv));
    goto RETURN_ERROR;
  }

  // Create socket
  if((socketfd = socket(sendinfo->ai_family, sendinfo->ai_socktype, sendinfo->ai_protocol)) == -1) {
    print(LogLevel::LOG_ERROR, "UDP send Error getting socket\n");
    goto RETURN_ERROR;
  }

  /////////////////////
  // SETUP UDP RECV PORT
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;
  
  // For recv address
  if((rv = getaddrinfo(hostname, recv_port, &hints, &recvinfo)) != 0) {
    print(LogLevel::LOG_ERROR, "UDP Error getting addrinfo: %s\n", gai_strerror(rv));
    goto RETURN_ERROR;
  }

  // Allow port reuse
  if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
    print(LogLevel::LOG_ERROR, "UDP setsockopt(SO_REUSEADDR) failed: %s\n", strerror(errno));
    goto RETURN_ERROR;
  }

  // Bind to port (tells the OS the default incoming address)
  if(bind(socketfd, recvinfo->ai_addr, recvinfo->ai_addrlen) != 0){
    print(LogLevel::LOG_ERROR, "UDP bind failed: %s\n", strerror(errno));
    goto RETURN_ERROR;
  }

  // Connect (tells the OS the default destination)
  // Connect to the send address
  //if(connect(socketfd, sendinfo->ai_addr, sendinfo->ai_addrlen) == -1) {
  //  print(LogLevel::LOG_ERROR, "UDP connecting failed: %s\n", strerror(errno));
  //  goto RETURN_ERROR;
  //}

  //freeaddrinfo(sendinfo);
  freeaddrinfo(recvinfo);
  return true;

RETURN_ERROR:
  print(LogLevel::LOG_ERROR, "UDP returning error on startup");
  close(socketfd);
  freeaddrinfo(sendinfo);
  freeaddrinfo(recvinfo);
  return false;
}

int UDPManager::udp_recv(uint8_t* recv_buf, uint8_t len){
  
  socklen_t fromlen;                // needed for recvfrom
  struct sockaddr_storage fromaddr; // needed for recvfrom
  fromlen = sizeof fromaddr;        // needed for recvfrom

  int byte_count = recvfrom(socketfd, recv_buf, len, 0, 
      (struct sockaddr *)&fromaddr, &fromlen);

  recv_buf[byte_count] = '\0';
  print(LogLevel::LOG_DEBUG, "recv %d bytes, they are: %s \n", byte_count, recv_buf);
  
  return byte_count;
}

bool UDPManager::udp_parse(uint8_t* buf, uint8_t len){
  if(buf[0] == 'P'){    //for an example, lets send the first byte to be P, for PING 
    return true;             //if we get ping, we know it's a dummy
  }
  else{
    print(LogLevel::LOG_INFO, "Parsed %d bytes, they are: %s\n",len, buf);   //Print whatever we got because it wasn't PING
  }
  return false;

}

int UDPManager::udp_send(uint8_t* buf, uint8_t len){ 
  int byte_count = sendto(socketfd, buf, len, 0,
      sendinfo->ai_addr, sendinfo->ai_addrlen);
  return byte_count;
}

void UDPManager::connection_monitor( const char * hostname, const char * send_port, const char * recv_port){

  // Create UDP socket
  if(!start_udp(hostname, send_port, recv_port)){
    print(LogLevel::LOG_ERROR, "Error setting up UDP\n");
    return; 
  }

  // Setup variables for UDP loop
  int rv;
  struct pollfd fds[1];
  fds[0].fd = socketfd;
  fds[0].events = POLLIN;
  int timeout = 1000; // 5 seconds TODO: set timeout
  
  /*
   * Timeout = -min(p) - min(D1) + T + max(D1) + max(p) 
   * p is processing time
   * D1 is delta 1: the time to receive the python's ping
   * T is the period at which this happens
   */




  // Run in a loop
  print(LogLevel::LOG_INFO, "UDP Setup complete\n");
  running.store(true);
  while (running){

    // More info about poll: 
    // http://beej.us/guide/bgnet/html/single/bgnet.html#indexId434909-276
    rv = poll(fds, 1, timeout);
    if( rv == -1){ // ERROR occured in poll()
      print(LogLevel::LOG_ERROR, "UDP poll() failed: %s\n", strerror(errno));
      //TODO: Once Unified Command Queue is implemented, consider this as a failure & write to queue
    }
    else if (rv == 0){
      // Timeout occured. No data after [timeout] ammount of time
      //TODO: Should we have recieved a PING during this time??????????
      // Option: Poll again in half the time then send a command to change to a safe state
      // How long can we not know what the pod is doing?

      //Something like this
/*
      print(LogLevel::LOG_INFO, "Re-Ping\n");
      int rvn = poll(fds, 1, timeout/2);
      if(rvn == 0){
	udp_send();//send something?	
      }
*/
      //TODO: What do we do ?
      // Should we PING again with a slower timeout, then if we don't see anything we break?
      print(LogLevel::LOG_DEBUG, "UDP timeout\n");
    }
    else{
      if(fds[0].revents & POLLIN){ // There is data to be read from UDP
        uint8_t buffer[] = {'A','C','K'};
        uint8_t buffer2[16];
        int byte_count = udp_recv(buffer2, sizeof(buffer2)); //TODO do something with this data we read
        bool ping = udp_parse(buffer2, byte_count);     //would we need connection_monitor to know what was parsed?
        byte_count = udp_send(buffer, sizeof(buffer));       //TODO what do we send??? 
        print(LogLevel::LOG_DEBUG, "sent %d bytes, \n", byte_count);
      }
      else{
        print(LogLevel::LOG_ERROR, "UDP poll event, but not on specified socket with specified event\n");
        //TODO: Once Unified Command Queue is implemented, consider this as a failure & write to queue
      }
    }
  }

  print(LogLevel::LOG_INFO, "UDP Exiting\n");
}

void UDPManager::close_client() {
  freeaddrinfo(sendinfo);
  shutdown(socketfd, SHUT_RDWR);
  close(socketfd);
}

void UDPManager::stop_threads() {
  running.store(false);
}