/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#if defined WIN32
#   include <WinSock2.h>
#   include <Windows.h>
#   include <stdio.h>
// FIXME: Where is socklen_t in Windows?
typedef int socklen_t;
#endif
#include <stdlib.h>
#include <fcntl.h>
#if defined HAVE_SYS_IOCTL_H
#   include <sys/ioctl.h>
#   include <sys/stat.h>
#   include <sys/types.h>
#include <signal.h>
#endif
#include <sys/types.h>

#if defined __APPLE__ && !defined __MACH__
#   include "GUSI.h"
#elif defined HAVE_NETINET_IN_H
#   include <netdb.h>
#   include <netinet/in.h>
#   include <stdio.h>
#   include <string.h>
#   include <sys/time.h>
#   include <sys/ipc.h>
#   include <sys/shm.h>
#   include <sys/socket.h>
#   include <unistd.h>
#   ifdef HAVE_BSTRING_H
#       include <bstring.h>
#   else
#       include <sys/select.h>
#   endif
#endif

#include "sock.h"
#include "isllist.h"

extern fd_set master_set, master_write_set, read_set, exception_set, write_set;

class ip_address : public net_address
{
public:
  sockaddr_in addr;

  virtual protocol protocol_type() const { return net_address::IP; }
  virtual int equal(const net_address *who) const
  //{ {{
  {
    if (who->protocol_type()==IP &&
        !memcmp(&addr.sin_addr,& ((ip_address const *)who)->addr.sin_addr,sizeof(addr.sin_addr)))
      return 1;
    else return 0;
  }
    //}}}
  virtual int set_port(int port)  { addr.sin_port=htons(port); return 1; }
  ip_address(sockaddr_in *Addr) { memcpy(&addr,Addr,sizeof(addr)); }
  virtual void print()
  //{ {{
  {
    unsigned char *c=(unsigned char *) (&addr.sin_addr.s_addr);
    fprintf(stderr,"%d.%d.%d.%d",c[0],c[1],c[2],c[3]);
  }
    //}}}
  int get_port() { return htons(addr.sin_port); }
  net_address *copy()  { return new ip_address(&addr); }
  ip_address() { } ;
  void store_string(char *st, int st_length)
  //{ {{
  {
    char buf[100];
    unsigned char *c=(unsigned char *) (&addr.sin_addr.s_addr);
    snprintf(buf,100,"%d.%d.%d.%d:%d",c[0],c[1],c[2],c[3],htons(addr.sin_port));
    strncpy(st,buf,st_length);
    st[st_length-1]=0;
  }
  //}}}
} ;

class tcpip_protocol : public net_protocol
{
protected:
  // Request Data
  struct RequestItem
  {
      ip_address *addr;
      char name[256];   //name
  };
  typedef isllist<RequestItem *>::iterator p_request;
  isllist<RequestItem*> servers,returned;

  // Notification Data
  net_socket *notifier;
  char notify_data[512];
  int notify_len;

  net_socket *responder;
  ip_address *bcast;

  int handle_notification();
  int handle_responder();
public :
  fd_set master_set,master_write_set,read_set,exception_set,write_set;

  tcpip_protocol();
  net_address *get_local_address();
  net_address *get_node_address(char const *&server_name, int def_port, int force_port);
  net_socket *connect_to_server(net_address *addr,
        net_socket::socket_type sock_type=net_socket::SOCKET_SECURE);
  net_socket *create_listen_socket(int port, net_socket::socket_type sock_type);
  int installed() { return 1; }  // always part of unix
  char const *name() { return "UNIX generic TCPIP"; }
  void cleanup();
  int select(int block);          // return # of sockets available for read & writing

  // Notification methods
  virtual net_socket *start_notify(int port, void *data, int len);
  virtual void end_notify();

  // Find notifiers methods
  virtual net_address *find_address(int port, char *name);   // name should be a 256 byte buffer
  virtual void reset_find_list();
  virtual ~tcpip_protocol() { cleanup(); }
} ;

extern tcpip_protocol tcpip;

class unix_fd : public net_socket
{
  protected :
  int fd;
  public :
  unix_fd(int fd) : fd(fd) { };
  virtual int error()                             { return FD_ISSET(fd,&tcpip.exception_set); }
  virtual int ready_to_read()                     { return FD_ISSET(fd,&tcpip.read_set); }
  virtual int ready_to_write()
  {
    struct timeval tv={ 0,0};     // don't wait
    fd_set write_check;
    FD_ZERO(&write_check);
    FD_SET(fd,&write_check);
    select(FD_SETSIZE,NULL,&write_check,NULL,&tv);
    return FD_ISSET(fd,&write_check);
  }
  virtual int write(void const *buf, int size, net_address *addr=NULL);
  virtual int read(void *buf, int size, net_address **addr);

#ifdef WIN32
  virtual ~unix_fd()                            { read_unselectable();  write_unselectable(); closesocket(fd); }
#else
  virtual ~unix_fd()                            { read_unselectable();  write_unselectable(); close(fd); }
#endif
  virtual void read_selectable()                   { FD_SET(fd,&tcpip.master_set); }
  virtual void read_unselectable()                 { FD_CLR(fd,&tcpip.master_set); }
  virtual void write_selectable()                  { FD_SET(fd,&tcpip.master_write_set); }
  virtual void write_unselectable()                { FD_CLR(fd,&tcpip.master_write_set); }
  int get_fd() { return fd; }

  void broadcastable();
} ;

class tcp_socket : public unix_fd
{
  int listening;
  public :
  tcp_socket(int fd) : unix_fd(fd) { listening=0; };
  virtual int listen(int port)
  {
    sockaddr_in host;
    memset( (char*) &host,0, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    host.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *) &host, sizeof(sockaddr_in))==-1)
    {
      fprintf(stderr,"net driver : could not bind socket to port %d\n",port);
      return 0;
    }
    if (::listen(fd,5)==-1)
    {
      fprintf(stderr,"net driver : could not listen to socket on port %d\n",port);
      return 0;
    }
    listening=1;
    return 1;
  }
  virtual net_socket *accept(net_address *&addr)
  {
    if (listening)
    {
      struct sockaddr_in from;
      socklen_t addr_len=sizeof(from);
      int new_fd=::accept(fd,(sockaddr *)&from,&addr_len);
      if (new_fd>=0)
      {
        addr=new ip_address(&from);
        return new tcp_socket(new_fd);
      }
      else
      { addr=NULL; return 0; }
    }
    return 0;
  }
} ;

class udp_socket : public unix_fd
{
  public :
  udp_socket(int fd) : unix_fd(fd) { };
  virtual int read(void *buf, int size, net_address **addr)
  {
    int tr;
    if (addr)
    {
      *addr=new ip_address;
      socklen_t addr_size=sizeof(sockaddr_in);
      tr=recvfrom(fd,(char*)buf,size,0, (sockaddr *) &((ip_address *)(*addr))->addr,&addr_size);
    } else
      tr=recv(fd,(char*)buf,size,0);
    return tr;
  }
  virtual int write(void const *buf, int size, net_address *addr=NULL)
  {
    if (addr)
      return sendto(fd,(char*)buf,size,0,(sockaddr *)(&((ip_address *)addr)->addr),sizeof(((ip_address *)addr)->addr));
    else {
#ifdef WIN32
      return send(fd, (char*)buf, size, 0);
#else
      return ::write(fd,(char*)buf,size);
#endif
    }
  }
  virtual int listen(int port)
  {
    sockaddr_in host;
    memset( (char*) &host,0, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    host.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *) &host, sizeof(sockaddr_in))==-1)
    {
      fprintf(stderr,"net driver : could not bind socket to port %d\n",port);
      return 0;
    }
    return 1;
  }

} ;
