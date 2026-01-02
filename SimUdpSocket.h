
#ifndef SIMUDPSOCKET__H
#define SIMUDPSOCKET__H

#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string>

#include <netinet/in.h>
#include <features.h> /* for the glibc version number */


#if __GLIBC__ >= 2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h> /* the L2 protocols */
#endif


                                                                                                                            
class CSimUdpSocket
{
public:
                                                                                                                            
   CSimUdpSocket();
   CSimUdpSocket(const char *IpAddr, int SendPort, int ReceivePort);
   ~CSimUdpSocket();

   bool Open(const char *IpAddr, int SendPort, int ReceivePort);

   int  SendToSocket(char *DataBuffer, int SizeInBytes);
   int  ReceiveFromSocket(char *DataBuffer, int MaxSizeToRead);
   int  ReceiveFromSocket(char *DataBuffer, int MaxSizeToRead, std::string &myIp);

   void SetNonBlockingFlag();
   void ClearNonBlockingFlag();

   int SetTtl(unsigned char ttl);
   int SetMultiCast(char *device_ip);
   int JoinMcastGroup(char *mcast_ip, char *device_ip);
   int DropMcastGroup(char *mcast_ip, char *device_ip);

private:

   bool   mIsOpen;
   int    mSocket;

   struct sockaddr_in mAddressOut;
   struct sockaddr_in mAddressIn;
};
                                                                                                                            
#endif

