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

#include <stdio.h>

#include "common.h"

#include "demo.h"
#include "specs.h"
#include "level.h"
#include "game.h"
#include "dev.h"
#include "timing.h"
#include "netface.h"

#if HAVE_NETWORK
#   include "fileman.h"
#endif
#include "net/sock.h"
#include "net/ghandler.h"
#include "net/gserver.h"
#include "net/gclient.h"
#include "dprint.h"
#include "netcfg.h"

/*

  This file is a combination of :
    src/net/unix/unixnfc.c
    src/net/unix/netdrv.c
    src/net/unix/undrv.c

    netdrv & undrv compile to a stand-alone program with talk with unixnfc
via a FIFO in /tmp, using a RPC-like scheme.  This versions runs inside
of a abuse and therefore is a bit simpler.
*/

base_memory_struct *base;   // points to shm_addr
base_memory_struct local_base;
net_address *net_server = NULL;
net_protocol *prot = NULL;
net_socket *comm_sock = NULL, *game_sock = NULL;
game_handler *game_face = NULL;
extern char lsf[256];
int local_client_number=0;        // 0 is the server
join_struct *join_array=NULL;      // points to an array of possible joining clients
extern char const *get_login();
extern void set_login(char const *name);


int net_init(int argc, char **argv)
{
    int i,x,db_level=0;
    base=&local_base;

    local_client_number=0;

    if (!main_net_cfg)
        main_net_cfg=new net_configuration;

    for (i=1; i<argc; i++)
    {
        if (!strcmp(argv[i],"-nonet"))
        {
            printf( "Net: Disabled (-nonet)\n" );
            return 0;
        }
        else if (!strcmp(argv[i],"-port"))
        {
            if( i == argc - 1 ||
                !sscanf( argv[i + 1], "%d", &x ) ||
                x < 1 || x > 0x7fff )
            {
                fprintf( stderr, "Net: Bad value following -port, use 1..32000\n" );
                return 0;
            }
            else
            {
                main_net_cfg->port = x;
            }
        }
        else if( !strcmp( argv[i], "-net" ) && i < argc-1 )
        {
            i++;
            strncpy(main_net_cfg->server_name, argv[i],
                    sizeof(main_net_cfg->server_name) - 1);
            main_net_cfg->server_name[sizeof(main_net_cfg->server_name) - 1]
                = '\0';
            main_net_cfg->state = net_configuration::CLIENT;
        }
        else if (!strcmp(argv[i],"-ndb"))
        {
            if (i==argc-1 || !sscanf(argv[i+1],"%d",&x) || x<1 || x>3)
            {
                fprintf(stderr,"Net: Bad value following -ndb, use 1..3\n" );
                return 0;
            }
            else
            {
                db_level = x;
            }
        }
        else if( !strcmp( argv[i], "-server" ) )
        {
            main_net_cfg->state = net_configuration::SERVER;
        }
        else if( !strcmp( argv[i], "-min_players" ) )
        {
            i++;
            int x = atoi( argv[i] );
            if (x >= 1 && x <= 8)
            {
                main_net_cfg->min_players=x;
            }
            else
            {
                fprintf(stderr,"bad value for min_players use 1..8\n");
            }
        }
    }

    net_protocol *n = net_protocol::first, *usable = NULL;     // find a usable protocol from installed list
    int total_usable = 0;
    for( ; n; n = n->next )                                    // show a list of usables, just to be cute
    {
        fprintf( stderr, "Protocol %s : ",n->installed() ? "Installed" : "Not_installed" );
        fprintf( stderr, "%s\n", n->name() );
        if( n->installed() )
        {
            total_usable++;
            usable=n;
        }
    }

    if (!usable)
    {
        fprintf(stderr,"Net: No network protocols installed\n");
        return 0;
    }
    prot=usable;
    prot->set_debug_printing((net_protocol::debug_type)db_level);

    if (main_net_cfg->state==net_configuration::SERVER)
        set_login(main_net_cfg->name);

    comm_sock=game_sock=NULL;
    if (main_net_cfg->state==net_configuration::CLIENT)
    {
        dprintf("Attempting to locate server %s, please wait\n",main_net_cfg->server_name);
        char const *sn=main_net_cfg->server_name;
        net_server=prot->get_node_address(sn,DEFAULT_COMM_PORT,0);
        if (!net_server)
        {
            dprintf(symbol_str("unable_locate"));
            exit(0);
        }
        dprintf("Server located!  Please wait while data loads....\n");
    }

#if HAVE_NETWORK
    fman = new file_manager(argc,argv,prot); // manages remote file access
#endif
    game_face = new game_handler;
    join_array=(join_struct *)malloc(sizeof(join_struct)*MAX_JOINERS);
    base->join_list=NULL;
    base->mem_lock=0;
    base->calc_crcs=0;
    base->get_lsf=0;
    base->wait_reload=0;
    base->need_reload=0;
    base->input_state=INPUT_COLLECTING;
    base->current_tick=0;
    base->packet.packet_reset();

    return 1;
}

int net_start()  // is the game starting up off the net? (i.e. -net hostname)
{
    return main_net_cfg && main_net_cfg->state == net_configuration::CLIENT;
}

int kill_net()
{
  if (game_face) delete game_face;  game_face=NULL;
  if (join_array) free(join_array);  join_array=NULL;
  if (game_sock) { delete game_sock; game_sock=NULL; }
  if (comm_sock) { delete comm_sock; comm_sock=NULL; }
#if HAVE_NETWORK
  delete fman;  fman=NULL;
#endif
  if (net_server) { delete net_server; net_server=NULL; }
  if (prot)
  {
    prot->cleanup();
    prot=NULL;
    return 1;
  }
  return 0;
}

void net_uninit()
{
  kill_net();
}


#if HAVE_NETWORK
int NF_set_file_server(net_address *addr)
{
  if (prot)
  {
    fman->set_default_fs(addr);
    net_socket *sock=prot->connect_to_server(addr,net_socket::SOCKET_SECURE);

    if (!sock) { printf("set_file_server::connect failed\n"); return 0; }
    uint8_t cmd=CLIENT_CRC_WAITER;
    if ( (sock->write(&cmd,1)!=1 && printf("set_file_server::writefailed\n")) ||
     (sock->read(&cmd,1)!=1  && printf("set_file_server::read failed\n")))        // wait for confirmation that crc's are written
    { delete sock; return 0; }
    delete sock;
    return cmd;
  } else return 0;
}

int NF_set_file_server(char const *name)
{
  if (prot)
  {
    net_address *addr=prot->get_node_address(name,DEFAULT_COMM_PORT,0);
    if (addr)
    {
      int ret=NF_set_file_server(addr);
      delete addr;
      return ret;
    } else return 0;
  }
  return 0;
}


int NF_open_file(char const *filename, char const *mode)
{
    if (prot)
        return fman->rf_open_file(filename,mode);
    return -2;
}


long NF_close(int fd)
{
  if (prot)
    return fman->rf_close(fd);
  return 0;
}

long NF_read(int fd, void *buf, long size)
{
  if (prot)
    return fman->rf_read(fd,buf,size);
  return 0;
}

long NF_filelength(int fd)
{
  if (prot)
    return fman->rf_file_size(fd);
  return 0;
}

long NF_seek(int fd, long offset)
{
  if (prot)
    return fman->rf_seek(fd,offset);
  return 0;
}

long NF_tell(int fd)
{
  if (prot)
    return fman->rf_tell(fd);
  return 0;
}
#endif


void service_net_request()
{
#if HAVE_NETWORK
  if (prot)
  {
    if (prot->select(0))  // anything happening net-wise?
    {
      if (comm_sock && comm_sock->ready_to_read())  // new connection?
      {
                net_address *addr;

                net_socket *new_sock=comm_sock->accept(addr);
                if (new_sock)
                {
                  uint8_t client_type;
                  if (new_sock->read(&client_type,1)!=1)
                  {
                    delete addr;
                    delete new_sock;
                  }
                  else
                  {
                    switch (client_type)
                    {
                      case CLIENT_NFS :
                      {
                                delete addr;
                                fman->add_nfs_client(new_sock);
                      } break;
                      case CLIENT_CRC_WAITER :
                      {
                                crc_manager.write_crc_file(NET_CRC_FILENAME);       // return 0 on failure
                                client_type=1;                                  // confirmation byte
                                new_sock->write(&client_type,1);
                                delete new_sock;                                // done with this socket now
                                delete addr;
                      } break;
                      case CLIENT_LSF_WAITER :          // wants to know which .lsp file to start with
                      {
                                uint8_t len=strlen(lsf);
                                new_sock->write(&len,1);
                                new_sock->write(lsf,len);
                                delete new_sock;
                                delete addr;
                      } break;
                      default :
                      {
                                if (game_face->add_client(client_type,new_sock,addr)==0)  // ask server or client to add new client
                                {
                                  delete addr;
                                  delete new_sock;
                                }
                      } break;
                    }
                  }
                }
      }
      if (!game_face->process_net())
      {
                delete game_face;
                game_face=new game_handler;
      }
      fman->process_net();
    }
  }
#endif // HAVE_NETWORK
}


int get_remote_lsf(net_address *addr, char *filename)  // filename should be 256 bytes
{
  if (prot)
  {
    net_socket *sock=prot->connect_to_server(addr,net_socket::SOCKET_SECURE);
    if (!sock) return 0;

    uint8_t ctype=CLIENT_LSF_WAITER;
    uint8_t len;

    if (sock->write(&ctype,1)!=1 ||
                sock->read(&len,1)!=1 || len==0 ||
                sock->read(filename,len)!=len)
    {
      delete sock;
      return 0;
    }

    delete sock;
    return 1;

  }
  return 0;
}

void server_check() { ; }

int request_server_entry()
{
  if (prot && main_net_cfg)
  {
    if (!net_server) return 0;

    if (game_sock) delete game_sock;
    dprintf("Joining game in progress, hang on....\n");

    game_sock=prot->create_listen_socket(main_net_cfg->port+2,net_socket::SOCKET_FAST);     // this is used for fast game packet transmission
    if (!game_sock) { if (comm_sock) delete comm_sock; comm_sock=NULL; prot=NULL; return 0; }
    game_sock->read_selectable();

    net_socket *sock=prot->connect_to_server(net_server,net_socket::SOCKET_SECURE);
    if (!sock)
    {
      fprintf(stderr,"unable to connect to server\n");
      return 0;
    }

    uint8_t ctype=CLIENT_ABUSE;
    uint16_t port=lstl(main_net_cfg->port+2),cnum;

    uint8_t reg;
    if (sock->write(&ctype,1)!=1 ||   // send server out game port
                sock->read(&reg,1)!=1)        // is remote engine registered?
    { delete sock; return 0; }

    if (reg==2)   // too many players
    {
      fprintf(stderr, "%s", symbol_str("max_players"));
      delete sock;
      return 0;
    }

    // make sure the server is registered or sync problems will occur
    if (!reg)
    {
      fprintf(stderr, "%s", symbol_str("server_not_reg"));
      delete sock;
      return 0;
    }

    char uname[256];
    if (get_login())
      strcpy(uname,get_login());
    else strcpy(uname,"unknown");
    uint8_t len=strlen(uname)+1;
    short nkills;

    if (sock->write(&len,1)!=1 ||
                sock->write(uname,len)!=len ||
                sock->write(&port,2)!=2  ||            // send server out game port
                sock->read(&port,2)!=2   ||            // read server's game port
                sock->read(&nkills,2)!=2 ||
                sock->read(&cnum,2)!=2   || cnum==0    // read player number (cannot be 0 because 0 is server)
                )
    { delete sock; return 0; }

    nkills=lstl(nkills);
    port=lstl(port);
    cnum=lstl(cnum);

    main_net_cfg->kills=nkills;
    net_address *addr=net_server->copy();
    addr->set_port(port);

    delete game_face;
    game_face=new game_client(sock,addr);
    delete addr;

    local_client_number=cnum;
    return cnum;
  }
  return 0;
}

int reload_start()
{
  if (prot)
    return game_face->start_reload();
  return 0;
}

int reload_end()
{
  if (prot)
    return game_face->end_reload();
  return 0;
}


void net_reload()
{
  if (prot)
  {
    if (net_server)
    {
      if (current_level)
        delete current_level;
      bFILE *fp;

      if (!reload_start()) return ;

      do {            // make sure server saves the file
                fp=open_file(NET_STARTFILE,"rb");
                if (fp->open_failure()) { delete fp; fp=NULL; }
      } while (!fp);

      spec_directory sd(fp);

#if 0
      spec_entry *e=sd.find("Copyright 1995 Crack dot Com, All Rights reserved");
      if (!e)
      {
                the_game->show_help("This level is missing copyright information, cannot load\n");
                current_level=new level(100,100,"untitled");
                the_game->need_refresh();
      }
      else
#endif
        current_level=new level(&sd,fp,NET_STARTFILE);

      delete fp;
      base->current_tick=(current_level->tick_counter()&0xff);

      reload_end();
    } else if (current_level)
    {

      join_struct *join_list=base->join_list;


      while (join_list)
      {

                view *f=player_list;
                for (; f && f->next; f=f->next);      // find last player, add one for pn
                int i,st=0;
                for (i=0; i<total_objects; i++)
                if (!strcmp(object_names[i],"START"))
                st=i;

                game_object *o=create(current_start_type,0,0);
                game_object *start=current_level->get_random_start(320,NULL);
                if (start) { o->x=start->x; o->y=start->y; }
                else { o->x=100; o->y=100; }

                f->next=new view(o,NULL,join_list->client_id);
                strcpy(f->next->name,join_list->name);
                o->set_controller(f->next);
                f->next->set_tint(f->next->player_number);
                if (start)
                current_level->add_object_after(o,start);
                else
                current_level->add_object(o);

                view *v = f->next;
                v->m_aa = ivec2(5);
                v->m_bb = ivec2(319, 199) - ivec2(5);
                join_list = join_list->next;
      }
      base->join_list=NULL;
      current_level->save(NET_STARTFILE,1);
      base->mem_lock=0;


      Jwindow *j=wm->CreateWindow(ivec2(0, yres / 2), ivec2(-1), new info_field(0, 0, 0, symbol_str("resync"),
                          new button(0, wm->font()->Size().y + 5, ID_NET_DISCONNECT,
                             symbol_str("slack"),NULL)),symbol_str("hold!"))
;



      wm->flush_screen();
      if (!reload_start()) return ;

      base->input_state=INPUT_RELOAD;    // if someone missed the game tick with the RELOAD data in it, make sure the get it

      // wait for all client to reload the level with the new players
      do
      {
                service_net_request();
                if (wm->IsPending())
                {
                  SDL_Event ev;
                  do
                  {
                    wm->get_event(ev);
                    if (ev.type==EV_MESSAGE && ev.message.id==ID_NET_DISCONNECT)
                    {
                      game_face->end_reload(1);
                      base->input_state=INPUT_PROCESSING;
                    }

                  } while (wm->IsPending());

                  wm->flush_screen();
                }

      } while (!reload_end());
      wm->close_window(j);
      unlink(NET_STARTFILE);

      base->input_state=INPUT_COLLECTING;

    }
  }
}


int client_number() { return local_client_number; }


void send_local_request()
{
  if (prot)
  {
    if (current_level)
      base->current_tick=(current_level->tick_counter()&0xff);
    game_face->add_engine_input();
  } else base->input_state=INPUT_PROCESSING;

}


void kill_slackers()
{
  if (prot)
  {
    if (!game_face->kill_slackers())
    {
      delete game_face;
      game_face=new game_handler();
    }
  }
}

int get_inputs_from_server(unsigned char *buf)
{
  if (prot && base->input_state!=INPUT_PROCESSING)      // if input is not here, wait on it
  {
    time_marker start;

    int total_retry=0;
    Jwindow *abort=NULL;

    while (base->input_state!=INPUT_PROCESSING)
    {
      if (!prot)
      {
    base->input_state=INPUT_PROCESSING;
    return 1;
      }
      service_net_request();

      time_marker now;                   // if this is taking to long, the packet was probably lost, ask for it to be resent

      if (now.diff_time(&start)>0.05)
      {
    if (prot->debug_level(net_protocol::DB_IMPORTANT_EVENT))
      fprintf(stderr,"(missed packet)");


    game_face->input_missing();
    start.get_time();

    total_retry++;
    if (total_retry==12000)    // 2 minutes and nothing
    {
      abort=wm->CreateWindow(ivec2(0, yres / 2), ivec2(-1, wm->font()->Size().y*4),
                   new info_field(0, 0, 0, symbol_str("waiting"),
                          new button(0, wm->font()->Size().y + 5, ID_NET_DISCONNECT,
                             symbol_str("slack"),NULL)),symbol_str("Error"));
      wm->flush_screen();
    }
      }
      if (abort)
      {
    if (wm->IsPending())
    {
      Event ev;
      do
      {
        wm->get_event(ev);
        if (ev.type==EV_MESSAGE && ev.message.id==ID_NET_DISCONNECT)
        {
          kill_slackers();
          base->input_state=INPUT_PROCESSING;
        }
      } while (wm->IsPending());

      wm->flush_screen();
    }
      }
    }

    if (abort)
    {
      wm->close_window(abort);
      the_game->reset_keymap();

    }
  }


  memcpy(base->last_packet.data,base->packet.data,base->packet.packet_size()+base->packet.packet_prefix_size());

  int size=base->packet.packet_size();
  memcpy(buf,base->packet.packet_data(),size);

  base->packet.packet_reset();
  base->mem_lock=0;

  return size;
}

int become_server(char *name)
{
  if (prot && main_net_cfg)
  {
    delete game_face;

    if (comm_sock) delete comm_sock;
    comm_sock=prot->create_listen_socket(main_net_cfg->port,net_socket::SOCKET_SECURE);   // this is used for incomming connections
    if (!comm_sock) { prot=NULL; return 0; }
    comm_sock->read_selectable();
        prot->start_notify(0x9090, name, strlen(name));  // should we define a new socket for notifiers?

    if (game_sock) delete game_sock;
    game_sock=prot->create_listen_socket(main_net_cfg->port+1,net_socket::SOCKET_FAST);     // this is used for fast game packet transmission
    if (!game_sock) { if (comm_sock) delete comm_sock; comm_sock=NULL; prot=NULL; return 0; }
    game_sock->read_selectable();

    game_face=new game_server;
    local_client_number=0;
    return 1;
  }
  return 0;
}

void read_new_views() { ; }

void wait_min_players()
{
    if (game_face)
        game_face->game_start_wait();
}
