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
#include <stdlib.h>
#include <fcntl.h>
#if defined HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include "common.h"

#include "gserver.h"
#include "netface.h"
#include "timing.h"
#include "netcfg.h"
#include "id.h"
#include "jwindow.h"
#include "input.h"
#include "dev.h"
#include "game.h"

extern base_memory_struct *base;
extern net_socket *comm_sock,*game_sock;

extern net_protocol *prot;
extern join_struct *join_array;
extern void service_net_request();

game_server::game_server()
{
    player_list = NULL;
    waiting_server_input = 1;
    reload_state = 0;
}

int game_server::total_players()
{
    player_client *fl = player_list;
    int total = 1;
    for( ; fl; fl = fl->next)
    {
        total++;
    }
    return total;
}

void game_server::game_start_wait()
{
  int last_count=0;
  Jwindow *stat=NULL;
  SDL_Event ev;
  int abort=0;
  while (!abort && total_players()<main_net_cfg->min_players)
  {
    if (last_count!=total_players())
    {
      if (stat) wm->close_window(stat);
      char msg[100];
      sprintf(msg,symbol_str("min_wait"),main_net_cfg->min_players-total_players());
      stat = wm->CreateWindow(ivec2(100, 50), ivec2(-1), new info_field(0, 0, ID_NULL, msg,
                       new button(0, wm->font()->Size().y * 2, ID_CANCEL,symbol_str("cancel_button"),NULL)  ));
      wm->flush_screen();
      last_count=total_players();
    }

    if (wm->IsPending())
    {
      do { wm->GetEvent(ev); } while (ev.type == SDL_MOUSEMOTION && wm->IsPending());
      wm->flush_screen();
      if (ev.type == ABUSE_EV_MESSAGE && ev.user.code == ID_CANCEL)
        abort=1;
    }

    service_net_request();
  }
  if (stat)
  {
    wm->close_window(stat);
    wm->flush_screen();
  }
}

game_server::player_client::~player_client()
{
  delete comm;
  delete data_address;
}

void game_server::check_collection_complete()
{
  player_client *c;
  int got_all=waiting_server_input==0;
  int add_deletes=0;
  for (c=player_list; c && got_all; c=c->next)
  {
    if (c->delete_me())
      add_deletes=1;
    else if (c->has_joined() && c->wait_input())
      got_all=0;
  }

  if (add_deletes)
  {
    player_client *last=NULL;
    for (c=player_list; c; )
    {
      if (c->delete_me())
      {
    base->packet.write_uint8(SCMD_DELETE_CLIENT);
    base->packet.write_uint8(c->client_id);
    if (c->wait_reload())
    {
      c->set_wait_reload(0);
      check_reload_wait();
    }

    if (last) last->next=c->next;
    else player_list=c->next;
    player_client *d=c;
    c=c->next;
    delete d;
      } else
      {
    last=c;
    c=c->next;
      }
    }
  }

  if (got_all)    // see if we have input from everyone, if so send it out
  {
    base->packet.calc_checksum();

    for (c=player_list; c; c=c->next)      // setup for next time, wait for all the input
    {
      if (c->has_joined())
      {
    c->set_wait_input(1);
    game_sock->write(base->packet.data,base->packet.packet_size()+base->packet.packet_prefix_size(),c->data_address);

      }
    }

    base->input_state=INPUT_PROCESSING; // tell engine to start processing
    game_sock->read_unselectable();    // don't listen to this socket until we are prepared to read next tick's game data
    waiting_server_input=1;
  }
}

void game_server::add_engine_input()
{
  waiting_server_input=0;
  base->input_state=INPUT_COLLECTING;
  base->packet.set_tick_received(base->current_tick);
  game_sock->read_selectable();    // we can listen for game data now that we have server input
  check_collection_complete();
}

void game_server::add_client_input(char *buf, int size, player_client *c)
{
  if (c->wait_input())  // don't add if we already have it
  {
    base->packet.add_to_packet(buf,size);
    c->set_wait_input(0);
    check_collection_complete();
  }
}

void game_server::check_reload_wait()
{
  player_client *d=player_list;
  for (; d; d=d->next)
   if (d->wait_reload()) return ;    // we are still waiting for someone to reload the game
  base->wait_reload=0;
}

int game_server::process_client_command(player_client *c)
{
  uint8_t cmd;
  if (c->comm->read(&cmd,1)!=1) return 0;
  switch (cmd)
  {
    case CLCMD_REQUEST_RESEND :
    {
      uint8_t tick;
      if (c->comm->read(&tick,1)!=1) return 0;

      fprintf(stderr,"request for resend tick %d (game cur=%d, pack=%d, last=%d)\n",
          tick,base->current_tick,base->packet.tick_received(),base->last_packet.tick_received());

      if (tick==base->last_packet.tick_received())
      {
    net_packet *pack=&base->last_packet;
    game_sock->write(pack->data,pack->packet_size()+pack->packet_prefix_size(),c->data_address);
      }
      return 1;
    } break;
    case CLCMD_RELOAD_START :
    {
      if (reload_state)   // already in reload state, notify client ok to start reloading
      {
        if (c->comm->write(&cmd,1)!=1)
      c->set_delete_me(1);
      } else c->set_need_reload_start_ok(1);
      return 1;
    } break;

    case CLCMD_RELOAD_END :
    {
      c->set_wait_reload(0);
      return 1;
    } break;
    case CLCMD_UNJOIN :
    {
      c->comm->write(&cmd,1);        // don't care weither this works or not
      c->set_delete_me(1);
      if (base->input_state==INPUT_COLLECTING)
        check_collection_complete();
    } break;
  }
  return 0;
}


int game_server::process_net()
{
  int ret=0;
  /**************************       Any game data waiting?       **************************/
  if ((base->input_state==INPUT_COLLECTING ||
       base->input_state==INPUT_RELOAD)
       && game_sock->ready_to_read())
  {
    net_packet tmp;
    net_packet *use=&tmp;
    net_address *from;
    int bytes_received=game_sock->read(use->data,PACKET_MAX_SIZE,&from);

    if (from && bytes_received)
    {
      // make sure we got a complete packet and the packet was not a previous game tick packet
      if (bytes_received==use->packet_size()+use->packet_prefix_size())
      {
    uint16_t rec_crc=use->get_checksum();
    if (rec_crc==use->calc_checksum())
    {
      player_client *f=player_list,*found=NULL;
      for (; !found &&f; f=f->next)
      if (f->has_joined() && from->equal(f->data_address))
        found=f;
      if (found)
      {
        if (base->current_tick==use->tick_received())
        {
          if (prot->debug_level(net_protocol::DB_MINOR_EVENT))
            fprintf(stderr,"(got data from %d)",found->client_id);

//          fprintf(stderr,"(got packet %d)\n",use->tick_received());
//          { time_marker now,start; while (now.diff_time(&start)<5.0) now.get_time(); }

          if (base->input_state!=INPUT_RELOAD)
            add_client_input((char *)use->packet_data(),use->packet_size(),found);

        }
        else if (use->tick_received()==base->last_packet.tick_received())
        {
          if (prot->debug_level(net_protocol::DB_IMPORTANT_EVENT))
            fprintf(stderr,"(sending old %d)\n",use->tick_received());

          // if they are sending stale data we need to send them the last packet so they can catchup
          net_packet *pack=&base->last_packet;
          game_sock->write(pack->data,pack->packet_size()+pack->packet_prefix_size(),found->data_address);

        } else if (prot->debug_level(net_protocol::DB_MAJOR_EVENT))
          fprintf(stderr,"received stale packet (got %d, expected %d)\n",use->tick_received(),base->current_tick);

      } else
      {
        if (prot->debug_level(net_protocol::DB_MAJOR_EVENT))
        {
          fprintf(stderr,"received data from unknown client\n");
          printf("from address "); from->print();
          printf(" first addr "); player_list->data_address->print(); printf("\n");
        }
      }

    } else fprintf(stderr,"received packet with bad checksum\n");
      } else fprintf(stderr,"received incomplete packet\n");
    } else if (!from)
      fprintf(stderr,"received data and no from\n");
    else if (!bytes_received)
      fprintf(stderr,"received 0 byte data\n");
    ret=1;
    if (from) delete from;

  }


  /**************************       Any client with commands?       **************************/
  player_client *c;
  for (c=player_list; c; c=c->next)
    if (c->comm->error() || (c->comm->ready_to_read() && !process_client_command(c)))
    {
      c->set_delete_me(1);
      check_collection_complete();
    }
    else ret=1;

  return 1;
}


int game_server::input_missing()
{

  return 1;
}



int game_server::end_reload(int disconnect)  // notify evryone you've reloaded the level (at server request)
{
  player_client *c=player_list;
  prot->select(0);

  for (; c; c=c->next)
    if (!c->delete_me() && c->wait_reload())
    {
      if (disconnect)
        c->set_delete_me(1);
      else return 0;
    }

  for (c=player_list; c; c=c->next)
    c->set_has_joined(1);
  reload_state=0;

  return 1;
}

int game_server::start_reload()
{
  player_client *c=player_list;
  reload_state=1;
  prot->select(0);

  for (; c; c=c->next)
  {
    if (!c->delete_me() && c->need_reload_start_ok())    // if the client is already waiting for reload state to start, send ok
    {
      uint8_t cmd=CLCMD_RELOAD_START;
      if (c->comm->write(&cmd,1)!=1) { c->set_delete_me(1); }
      c->set_need_reload_start_ok(0);
    }
    c->set_wait_reload(1);
  }
  return 1;
}


int game_server::isa_client(int client_id)
{
  player_client *c=player_list;
  if (!client_id) return 1;
  for (; c; c=c->next) if (c->client_id==client_id) return 1;
  return 0;
}

int game_server::add_client(int type, net_socket *sock, net_address *from)
{
    if( type == CLIENT_ABUSE )
    {
        if( total_players() >= main_net_cfg->max_players )
        {
            uint8_t too_many = 2;
            sock->write( &too_many, 1 );
            return 0;
        }

        uint8_t reg = 1; // Of course the game is registered
        if( sock->write( &reg, 1 ) != 1 )
            return 0;

        uint16_t our_port = lstl( main_net_cfg->port + 1 ), cport;
        char name[256];
        uint8_t len;
        int16_t nkills=lstl(main_net_cfg->kills);

        if( sock->read(&len,1)!=1 ||
            sock->read(name,len)!=len ||
            sock->read(&cport,2)!=2 ||
            sock->write(&our_port,2)!=2 ||
            sock->write(&nkills,2)!=2)
        {
            return 0;
        }

        cport=lstl(cport);

        int f = -1, i;
        for( i = 0; f == -1 && i < MAX_JOINERS; i++ )
        {
            if( !isa_client(i) )
            {
                f = i;
                join_struct *j=base->join_list;
                for( ; j; j = j->next )
                {
                    if( j->client_id == i )
                        f = -1;
                }
            }
        }

        if( f == -1 )
            return 0;

        from->set_port( cport );

        uint16_t client_id = lstl( f );
        if( sock->write( &client_id, 2 ) != 2 )
        {
            return 0;
        }
        client_id=f;

        join_array[client_id].next = base->join_list;
        base->join_list = &join_array[client_id];
        join_array[client_id].client_id = client_id;
        strcpy( join_array[client_id].name, name );
        player_list = new player_client( f, sock, from, player_list );

        return 1;
    }
    else
    {
        return 0;
    }
}

int game_server::kill_slackers()
{
  player_client *c=player_list;
  for (; c; c=c->next)
    if (c->wait_input())
      c->set_delete_me(1);
  check_collection_complete();
  return 1;
}

int game_server::quit()
{
  player_client *c=player_list;
  while (c)
  {
    player_client *d=c;
    c=c->next;
    delete d;
  }
  player_list=NULL;
  return 1;
}


game_server::~game_server()
{
    quit();
}

