/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_NETWORK

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#if defined HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <SDL3/SDL_stdinc.h>
#ifdef WIN32
# include <io.h>
#endif

#include "common.h"

#include "fileman.h"
#include "netface.h"
#include "ghandler.h"
#include "specache.h"

extern net_protocol *prot;

// FIXME: Where is this supposed to be coming from?
typedef unsigned short ushort;


file_manager *fman=NULL;

file_manager::file_manager(int argc, char **argv, net_protocol *proto) : proto(proto)
{
  default_fs=NULL;
  no_security=0;
  nfs_list=NULL;

  int i;
  for (i=1; i<argc; i++)
    if (!strcmp(argv[i],"-bastard"))   // this bypasses filename security features
    {
      fprintf(stderr,"Warning : Security measures bypassed (-bastard)\n");
      no_security=1;
    }
}



void file_manager::process_net()
{
  nfs_client *nc,*last=NULL;
  for (nc=nfs_list; nc; )      // check for nfs request
  {

    int ok=1;

    if (nc->sock->error())
    {
      ok=0;
      //fprintf(stderr,"Killing nfs client, socket went bad\n");
    }
    else if (nc->size_to_read && nc->sock->ready_to_write())
      ok=nc->send_read();
    else if (nc->sock->ready_to_read())
      ok=process_nfs_command(nc);    // if we couldn't process the packet, delete the connection

    if (ok)
    {
      last=nc;
      nc=nc->next;
    } else
    {
      if (last) last->next=nc->next;
      else nfs_list=nc->next;
      nfs_client *c=nc;
      nc=nc->next;
      delete c;
    }
  }
}


int file_manager::process_nfs_command(nfs_client *c)
{
  char cmd;
  if (c->sock->read(&cmd,1)!=1) return 0;
  switch (cmd)
  {
    case NFCMD_READ :
    {
      int32_t size;
      if (c->sock->read(&size,sizeof(size))!=sizeof(size)) return 0;
      size=lltl(size);

      c->size_to_read=size;
      return c->send_read();
    } break;
    case NFCMD_CLOSE :
    {
      return 0;
    } break;
    case NFCMD_SEEK :
    {
      int32_t offset;
      if (c->sock->read(&offset,sizeof(offset))!=sizeof(offset)) return 0;
      offset=lltl(offset);
      offset=lseek(c->file_fd,offset,0);
      offset=lltl(offset);
      if (c->sock->write(&offset,sizeof(offset))!=sizeof(offset)) return 0;
      return 1;
    } break;
    case NFCMD_TELL :
    {
      int32_t offset=lseek(c->file_fd,0,SEEK_CUR);
      offset=lltl(offset);
      if (c->sock->write(&offset,sizeof(offset))!=sizeof(offset)) return 0;
      return 1;
    } break;

    default :
    { fprintf(stderr,"net driver : bad command from nfs client\n");
      return 0;
    }
  }
}

int file_manager::nfs_client::send_read()   // return 0 if failure on socket, not failure to read
{
  if (file_fd>=0 && sock)
  {
    // first make sure the socket isn't 'full'
    if (sock->ready_to_write())
    {
      char buf[READ_PACKET_SIZE];
      short read_total;
      short actual;

      do
      {
    read_total=size_to_read>(READ_PACKET_SIZE-2) ? (READ_PACKET_SIZE-2) : size_to_read;

    actual=read(file_fd,buf+2,read_total);
    ushort tmp = lstl(actual);
    memcpy(buf, &tmp, sizeof(tmp));

    int write_amount=sock->write(buf,actual+2);
    if (write_amount!=actual+2)
    {
      fprintf(stderr,"write failed\n");
      return 0;
    }

    size_to_read-=actual;

    if (!sock->ready_to_write())
    {
      sock->read_unselectable();
      sock->write_selectable();
      return 1;    // not ok to write anymore, try again latter
    }

      } while (size_to_read && actual==read_total);

      sock->read_selectable();
      sock->write_unselectable();

      size_to_read=0;
      return 1;
    } else
    {
      sock->read_unselectable();
      sock->write_selectable();
      return 1;
    }
  }
  return 0;
}


void file_manager::secure_filename(char *filename, char *mode)
{
  if (!no_security)
  {
    if (filename[0]=='/') { filename[0]=0; return ; }
    int level=0;
    char *f=filename;
    while (*f)
    {
      if (*f=='/') { f++; level++; }
      else if (*f=='.' && f[1]=='.')
      {
    if (f[3]=='.') while (*f!='.') f++;
    else
    {
      f+=2;
      level--;
    }
      } else f++;

    }
    if (level<0)
      filename[0]=0;
  }
}




file_manager::nfs_client::nfs_client(net_socket *sock, int file_fd, nfs_client *next) :
  sock(sock),file_fd(file_fd),next(next),size_to_read(0)
{
  sock->read_selectable();
}


file_manager::nfs_client::~nfs_client()
{
  delete sock;
  if (file_fd>=0)
    close(file_fd);
}


void file_manager::add_nfs_client(net_socket *sock)
{
  uint8_t size[2];
  char filename[300],mode[20],*mp;
  if (sock->read(size,2)!=2) { delete sock; return ; }
  if (sock->read(filename,size[0])!=size[0]) { delete sock; return ; }
  if (sock->read(mode,size[1])!=size[1]) { delete sock; return ; }


  secure_filename(filename,mode);  // make sure this filename isn't a security risk
  if (filename[0]==0) { fprintf(stderr,"(denied)\n"); delete sock; return ; }

  mp=mode;
  int flags=0;

  while (*mp)
  {
    if (*mp=='w') flags|=O_CREAT|O_RDWR;
    else if (*mp=='r') flags|=O_RDONLY;
    mp++;
  }

#ifdef WIN32
  int f=open(filename,flags,_S_IREAD | _S_IWRITE);
#else
  int f=open(filename,flags,S_IRWXU | S_IRWXG | S_IRWXO);
#endif

  FILE *fp=fopen("open.log","ab");
  fprintf(fp,"open file %s, fd=%d\n",filename,f);
  fclose(fp);

  if (f<0)
    f=-1;  // make sure this is -1


  int32_t ret=lltl(f);
  if (sock->write(&ret,sizeof(ret))!=sizeof(ret)) { delete sock; return ; }

  if (f<0)    // no file, sorry
    delete sock;
  else
  {
    int32_t cur_pos=lseek(f,0,SEEK_CUR);
    int32_t size=lseek(f,0,SEEK_END);
    lseek(f,cur_pos,SEEK_SET);
    size=lltl(size);
    if (sock->write(&size,sizeof(size))!=sizeof(size)) {  close(f); delete sock; sock=NULL; return ; }

    nfs_list=new nfs_client(sock,f,nfs_list);
    nfs_list->size=size;
  }
}



void file_manager::remote_file::r_close(char const *reason)
{
//  if (reason) fprintf(stderr,"remote_file : %s\n",reason);

  if (sock)
  {
    delete sock;
    sock=NULL;
  }

}

file_manager::remote_file::remote_file(net_socket *sock, char const *filename, char const *mode, remote_file *Next) : sock(sock)
{
  next=Next;
  open_local=0;

  size_t filename_len = strlen(filename) + 1;
  size_t mode_len = strlen(mode) + 1;
  if (filename_len > SDL_MAX_UINT8 || mode_len > SDL_MAX_UINT8) { r_close("filename/mode too long"); return; }
  uint8_t sizes[3]={ CLIENT_NFS, (uint8_t) filename_len, (uint8_t) mode_len};
  if (sock->write(sizes,3)!=3) { r_close("could not send open info"); return ; }
  if (sock->write(filename,sizes[1])!=sizes[1]) { r_close("could not send filename"); return ; }
  if (sock->write(mode,sizes[2])!=sizes[2]) { r_close("could not send mode"); return ; }

  int32_t remote_file_fd;
  if (sock->read(&remote_file_fd,sizeof(remote_file_fd))!=sizeof(remote_file_fd))
  { r_close("could not read remote fd"); return ; }
  remote_file_fd=lltl(remote_file_fd);
  if (remote_file_fd<0) { r_close("remote fd is bad"); return ; }

  if (sock->read(&size,sizeof(size))!=sizeof(size)) { r_close("could not read remote filesize"); return ; }

  size=lltl(size);
}

int file_manager::remote_file::unbuffered_read(void *buffer, size_t count)
{
  if (sock && count)
  {
    uint8_t cmd=NFCMD_READ;
    if (sock->write(&cmd,sizeof(cmd))!=sizeof(cmd)) { r_close("read : could not send command"); return 0; }

    int32_t rsize=lltl(count);
    if (sock->write(&rsize,sizeof(rsize))!=sizeof(rsize)) { r_close("read : could not send size"); return 0; }

    int32_t total_read=0;
    char buf[READ_PACKET_SIZE];

    ushort packet_size;
    do
    {
      if (sock->read(&packet_size,sizeof(packet_size))!=sizeof(packet_size))
      {
    fprintf(stderr,"could not read packet size\n");
    return 0;
      }

      packet_size=lstl(packet_size);

      ushort size_read=sock->read(buf,packet_size);

      if (size_read!=packet_size)
      {
    if (sock->read(buf+2+size_read,packet_size-size_read)!=packet_size-size_read)
    {
      fprintf(stderr,"incomplete packet\n");
      return 0;
    }
      }

      memcpy(buffer,buf,packet_size);
      buffer=(void *)(((char *)buffer)+packet_size);

      total_read+=packet_size;
      count-=packet_size;
    } while (packet_size==READ_PACKET_SIZE-2 && count);
    return total_read;
  }
  return 0;
}

int32_t file_manager::remote_file::unbuffered_tell()   // ask server where the offset of the file pointer is
{
  if (sock)
  {
    uint8_t cmd=NFCMD_TELL;
    if (sock->write(&cmd,sizeof(cmd))!=sizeof(cmd)) { r_close("tell : could not send command"); return 0; }

    int32_t offset;
    if (sock->read(&offset,sizeof(offset))!=sizeof(offset)) { r_close("tell : could not read offset"); return 0; }
    return lltl(offset);
  }
  return 0;
}

int32_t file_manager::remote_file::unbuffered_seek(int32_t offset)  // tell server to seek to a spot in a file
{
  if (sock)
  {
    uint8_t cmd=NFCMD_SEEK;
    if (sock->write(&cmd,sizeof(cmd))!=sizeof(cmd)) { r_close("seek : could not send command"); return 0; }

    int32_t off=lltl(offset);
    if (sock->write(&off,sizeof(off))!=sizeof(off)) { r_close("seek : could not send offset"); return 0; }

    if (sock->read(&offset,sizeof(offset))!=sizeof(offset)) { r_close("seek : could not read offset"); return 0; }
    return lltl(offset);
  }
  return 0;
}


file_manager::remote_file::~remote_file()
{ r_close(NULL); }

int file_manager::rf_open_file(char const *&filename, char const *mode)
{
  net_address *fs_server_addr=NULL;

  if (filename[0]=='/' && filename[1]=='/')   // passive server file reference?
  {
    filename+=2;

    fs_server_addr=prot->get_node_address(filename,DEFAULT_COMM_PORT,0);
    if (!fs_server_addr)
    {
      printf("could not get address for %s\n",filename);
      return -1;
    }
  } else if (default_fs)
    fs_server_addr=default_fs->copy();

  if (fs_server_addr)
  {
    net_socket *sock=proto->connect_to_server(fs_server_addr,net_socket::SOCKET_SECURE);
    delete fs_server_addr;

    if (!sock)
    {
      fprintf(stderr,"unable to connect\n");
      return -1;
    }

    remote_file *rf=new remote_file(sock,filename,mode,remote_list);
    if (rf->open_failure())
    {
      delete rf;
      return -1;
    }
    else
    {
      remote_list=rf;
      return rf->sock->get_fd();
    }
  }


// FIXME: AK - Should we keep this?
//  secure_filename(filename,mode);
  if (filename[0]==0) return -1;

  int flags=0;
  while (*mode)
  {
    if (*mode=='w')
    {
      flags|=O_CREAT|O_RDWR;
    }
    else if (*mode=='r')
    {
      flags|=O_RDONLY;
    }
#ifdef WIN32
    else if (*mode=='b')
    {
      flags|=O_BINARY;
    }
#endif
    mode++;
  }

  char tmp_name[200];
#ifdef WIN32
  if (get_filename_prefix() && filename[0] != '/' && (filename[0] != '\0' && filename[1] != ':'))
#else
  if (get_filename_prefix() && filename[0] != '/')
#endif
  {
    snprintf(tmp_name,200,"%s%s",get_filename_prefix(),filename);
  }
  else
  {
    strcpy(tmp_name,filename);
  }

#ifdef WIN32
  int f = open(tmp_name, flags, S_IREAD|S_IWRITE);
#else
  int f=open(tmp_name,flags,S_IRWXU | S_IRWXG | S_IRWXO);
#endif
  if (f>=0)
  { close(f);
    return -2;
  }

  return -1;
}


file_manager::remote_file *file_manager::find_rf(int fd)
{
  remote_file *r=remote_list;
  for (; r && r->sock->get_fd()!=fd; r=r->next)
  {
    if (r->sock->get_fd()==-1)
    {
      fprintf(stderr,"bad sock\n");
    }
  }
  if (!r) { fprintf(stderr,"Bad fd for remote file %d\n",fd); }
  return r;
}


int32_t file_manager::rf_tell(int fd)
{
  remote_file *rf=find_rf(fd);
  if (rf) return rf->unbuffered_tell();
  else return 0;
}

int32_t file_manager::rf_seek(int fd, int32_t offset)
{
  remote_file *rf=find_rf(fd);
  if (rf) return rf->unbuffered_seek(offset);
  else return 0;
}

int file_manager::rf_read(int fd, void *buffer, size_t count)
{
  remote_file *rf=find_rf(fd);
  if (rf) return rf->unbuffered_read(buffer,count);
  else return 0;
}

int file_manager::rf_close(int fd)
{
  remote_file *rf=remote_list,*last=NULL;
  while (rf && rf->sock->get_fd()!=fd) rf=rf->next;
  if (rf)
  {
    if (last) last->next=rf->next;
    else remote_list=rf->next;
    delete rf;
    return 1;
  } else
  {
    fprintf(stderr,"Bad fd for remote file %d\n",fd);
    return 0;
  }
}

int32_t file_manager::rf_file_size(int fd)
{
  remote_file *rf=find_rf(fd);
  if (rf) return rf->file_size();
  else return 0;
}

#endif // HAVE_NETWORK
