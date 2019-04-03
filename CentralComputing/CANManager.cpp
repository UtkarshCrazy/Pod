#include "CANManager.h"

bool CANManager::initialize_source() {
  #ifndef BBB
  print(LogLevel::LOG_ERROR, "CAN Manager setup failed, not on BBB\n");
  return false;
  #endif

  // Setup structs that will be used below
  memset(&ifr,   0, sizeof(ifr));
  memset(&addr,  0, sizeof(addr));

  // Open CAN_RAW socket
  if((can_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) == -1){
    print(LogLevel::LOG_ERROR, "CAN Socket creation failed. %s\n", strerror(errno));
    return false;
  }

  // Convert interface string into interface index
  strcpy(ifr.ifr_name, "can0");
  if(ioctl(can_fd, SIOCGIFINDEX, &ifr) == -1){
    print(LogLevel::LOG_ERROR, "CAN ioctl SIOCGIFINDEX failed. %s\n", strerror(errno));
    return false;
  }

  // Setup address for Bind
  addr.can_ifindex = ifr.ifr_ifindex;
  addr.can_family  = PF_CAN;

  // Bind
  if(bind(can_fd, (struct sockaddr *) & addr, sizeof(addr)) == -1){
    print(LogLevel::LOG_ERROR, "CAN bind failed. %s\n", strerror(errno));
    return false;
  }

  // Setup recv_frame() variables
  iov.iov_base = &r_frame;
  msg.msg_name = &addr;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = &ctrlmsg;

  print(LogLevel::LOG_DEBUG, "CAN Manger setup successful\n");
  return true;
}

void CANManager::stop_source(){
  close(can_fd);
  print(LogLevel::LOG_DEBUG, "CAN Manger stopped\n");
}

bool CANManager::send_frame(uint32_t can_id, const char * buf, int len){
  // Populate frame
  s_frame.can_id = can_id;
  strncpy((char *)s_frame.data, buf, (size_t)len);
  s_frame.can_dlc = len;
  // Write s_frame
  int ret = write(can_fd, &s_frame, sizeof(s_frame));
  // Check for errors
  if(ret == -1){
    print(LogLevel::LOG_ERROR, "CAN send_frame failed. %s\n", strerror(errno));
    return false;
  }
  else if(ret == 0){
    print(LogLevel::LOG_ERROR, "CAN send_frame sent 0 bytes\n");
    return false;
  }
  return true;
}

bool CANManager::recv_frame(){
  //https://stackoverflow.com/questions/32593697/understanding-the-msghdr-structure-from-sys-socket-h#32594071

  iov.iov_len = sizeof(r_frame);
  msg.msg_namelen = sizeof(addr);
  msg.msg_controllen = sizeof(ctrlmsg);
  msg.msg_flags = 0;

  int byte_count = recvmsg(can_fd, &msg, MSG_DONTWAIT); 

  if(byte_count == -1){
    r_frame.len = 0;
    r_frame.can_id = 0;
    if(errno == EAGAIN || errno == EWOULDBLOCK){
      print(LogLevel::LOG_DEBUG, "CAN eagain ewouldblock. %s\n", strerror(errno));
      return true;
    }
    else{
      print(LogLevel::LOG_ERROR, "CAN recvmsg failed. %s\n", strerror(errno));
      return false;
    }
  }

  // Check to make sure we read a full message
  // See linux/can.h for more details on these constants
	if((size_t)byte_count == CAN_MTU){
    print(LogLevel::LOG_DEBUG, "Can MTU\n");
  }
  else if((size_t)byte_count == CANFD_MTU){
    print(LogLevel::LOG_DEBUG, "CanFD MTU\n");
  }
  else {
    print(LogLevel::LOG_ERROR, "CAN recvmsg failed, incomplete CAN frame. \n");
    return false;
  }
  return true;

}

std::shared_ptr<CANData> CANManager::refresh() {
  std::shared_ptr<CANData> new_data = std::make_shared<CANData>();

  long long a = Utils::microseconds();
  //Send test frame 
  if(!send_frame(100, "wow", 3)){
    print(LogLevel::LOG_ERROR, "CAN send_frame failed. \n");
    //TODO: put error on unified command queue
  }
  long long b = Utils::microseconds();
  print(LogLevel::LOG_INFO, "CAN send_frame takes %lu microseconds\n", b-a);

  //Recieve frame(s). Populates variable r_frame
  do{
    a = Utils::microseconds();
    if(!recv_frame()){
      print(LogLevel::LOG_ERROR, "CAN recv_frame failed. \n");
      //TODO: put error on unified command queu
    }
    b = Utils::microseconds();
    print(LogLevel::LOG_INFO, "CAN recv_frame takes %lu microseconds\n", b-a);

    //Print the contents of r_frame (assumes len <= 8)
    char buff[16];
    for(int j = 0; j < r_frame.len*2; j+=2){
      put_hex_byte(buff+j, r_frame.data[j/2]);
    }
    buff[r_frame.len*2] = '\0';//include null terminator

    print(LogLevel::LOG_INFO, "CAN msg: id: %d, len: %d, data: %s\n", r_frame.can_id, r_frame.len, buff);
  }
  while(r_frame.len != 0);

  return new_data;
}

std::shared_ptr<CANData> CANManager::refresh_sim() {
  return empty_data();
}
