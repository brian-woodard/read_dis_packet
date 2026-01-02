
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "SimUdpSocket.h"

using namespace std;


CSimUdpSocket::CSimUdpSocket()
{

   // initialize
   mIsOpen = false;
}


CSimUdpSocket::CSimUdpSocket(const char *IpAddress, int SendPort, int RecvPort)
{

   // open the socket
   Open(IpAddress, SendPort, RecvPort);
}

// ----------------------------------------------------------------------------------------------------
CSimUdpSocket::~CSimUdpSocket()
{
   close (mSocket);
}


bool CSimUdpSocket::Open(const char *IpAddress, int SendPort, int RecvPort)
{

   int optval = 1;

   // check if already open
   if (mIsOpen) return true;

   // Initialize the output socket structure
   memset (&mAddressOut, 0, sizeof(mAddressOut));
   mAddressOut.sin_addr.s_addr = inet_addr(IpAddress);
   mAddressOut.sin_family      = AF_INET;
   mAddressOut.sin_port        = htons(SendPort);

   // Initialize the input socket structure
   memset (&mAddressIn, 0, sizeof(mAddressIn));
   mAddressIn.sin_addr.s_addr = INADDR_ANY;
   mAddressIn.sin_family      = AF_INET;
   mAddressIn.sin_port        = htons(RecvPort);
  
  // open the sockets
  if ( (mSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
    perror(" UDP Port Constructor: Output socket()");
    return false;
  }

  // Set the socket options for broadcast address
  if (setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0) {
    perror(" UDP Port Constructor: SetSockOpt()");
    return false;
  }

  if (setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    perror(" UDP Port Constructor: SetSockOpt()");
    return false;
  }

  //  Bind to the  input socket 
  if (bind (mSocket, (struct sockaddr*)&mAddressIn, sizeof(mAddressIn)) == -1 )  {
     perror("UDP Port Constructor(): bind()");
     return false;
  }

  return true;
}


// ----------------------------------------------------------------------------------------------------

int CSimUdpSocket::SendToSocket(char *DataBuffer, int SizeInBytes) 
{

  int msg_size;
  int send_status;  /* Status of sendto function call */



  /* Send data out on the socket */
  send_status = sendto(mSocket, DataBuffer, SizeInBytes, 0, (struct sockaddr*)&mAddressOut,sizeof(mAddressOut));
    
  if ( send_status < 0 ) {
//      perror("WriteDataToPort(): sendto()");
      return (-1);
  }

  return 0;
}

// ----------------------------------------------------------------------------------------------------

int  CSimUdpSocket::ReceiveFromSocket(char *DataBuffer, int MaxSizeToRead)
{
  
  char msg[64000];            /* Plenty of room */
  int msg_size = 64000;
  int bytes_returned;
  int fromlen;
  struct sockaddr insock;

  
  fromlen = MaxSizeToRead;  /* This will get changed to actual number of bytes read in */

  /* Read any data received on the socket. */
  bytes_returned = recvfrom (mSocket, DataBuffer, MaxSizeToRead, 0, (struct sockaddr*)&insock,(socklen_t*)&fromlen);

  if (bytes_returned == -1)
  {
     if (errno == EAGAIN)
        return -3;
     else {
//        perror("HostServices::ReceiveFromSocket(): recvfrom()");
        return -1;
     }
  }

  return(bytes_returned);
}

int  CSimUdpSocket::ReceiveFromSocket(char *DataBuffer, int MaxSizeToRead, std::string &myIp)
{
  
  char msg[64000];            /* Plenty of room */
  int msg_size = 64000;
  int bytes_returned;
  int fromlen;
  struct sockaddr_in insock;
  std::string fromIp;
  
  fromlen = MaxSizeToRead;  /* This will get changed to actual number of bytes read in */

  /* Read any data received on the socket. */
  bytes_returned = recvfrom (mSocket, DataBuffer, MaxSizeToRead, 0, (struct sockaddr*)&insock,(socklen_t*)&fromlen);

  if (bytes_returned == -1)
  {
     if (errno == EAGAIN)
        return -3;
     else {
//        perror("HostServices::ReceiveFromSocket(): recvfrom()");
        return -1;
     }
  }

  fromIp = inet_ntoa(insock.sin_addr);

  if (fromIp == myIp) return 0;
  
  return(bytes_returned);
}

void CSimUdpSocket::SetNonBlockingFlag()
{
   int socket_flags;
 
   // Get Socket flags
   socket_flags = fcntl(mSocket,F_GETFL);

   // Set non-blocking status
   socket_flags |= FNDELAY;

   // Re-set the socket flags
   fcntl(mSocket,F_SETFL, socket_flags);
}

void CSimUdpSocket::ClearNonBlockingFlag()
{
   int socket_flags;
 
   // Get Socket flags
   socket_flags = fcntl(mSocket,F_GETFL);

   // Set blocking status
   socket_flags &= ~FNDELAY;

   // Re-set the socket flags
   fcntl(mSocket,F_SETFL, socket_flags);
}

int CSimUdpSocket::SetTtl(unsigned char ttl)
{
   unsigned char theTtl = ttl;

   int status = setsockopt(mSocket, IPPROTO_IP, IP_MULTICAST_TTL, &theTtl, sizeof(theTtl));

   return status;
}

int CSimUdpSocket::SetMultiCast(char *device_ip)
{
   struct in_addr interface_addr;
   interface_addr.s_addr = inet_addr(device_ip); 

   int status = setsockopt(mSocket, IPPROTO_IP, IP_MULTICAST_IF, &interface_addr, sizeof(interface_addr));

   return status;
}

int CSimUdpSocket::JoinMcastGroup(char *mcast_ip, char *device_ip)
{
   struct ip_mreq mreq;

   mreq.imr_multiaddr.s_addr = inet_addr(mcast_ip);
   mreq.imr_interface.s_addr  =inet_addr(device_ip);
   int status = setsockopt(mSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

   return status;
}

int CSimUdpSocket::DropMcastGroup(char *mcast_ip, char *device_ip)
{
   struct ip_mreq mreq;

   mreq.imr_multiaddr.s_addr = inet_addr(mcast_ip);
   mreq.imr_interface.s_addr  =inet_addr(device_ip);
   int status = setsockopt(mSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));

   return status;
}


