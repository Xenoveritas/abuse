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

#include "common.h"

#include "game.h"

#include "demo.h"
#include "specs.h"
#include "jwindow.h"
#include "dprint.h"
#include "dev.h"
#include "jrand.h"
#include "lisp.h"
#include "clisp.h"
#include "netface.h"


demo_manager demo_man;
ivec2 last_demo_mpos;
int last_demo_mbut;
extern base_memory_struct *base;   // points to shm_addr
extern int idle_ticks;

extern void net_receive();
extern void net_send(int force);
extern void fade_in(image *im, int steps);
extern void fade_out(int steps);

void get_event(SDL_Event &ev)
{ wm->get_event(ev);
  switch (ev.type)
  {
    case SDL_KEYDOWN:
    {
      if (demo_man.state==demo_manager::PLAYING)
        demo_man.set_state(demo_manager::NORMAL);
      else if (ev.key.keysym.sym==SDLK_RETURN && demo_man.state==demo_manager::RECORDING)
      {
        demo_man.set_state(demo_manager::NORMAL);
    the_game->show_help("Finished recording");
      }
    } break;
  }

  last_demo_mpos = ev.mouse_move;
  last_demo_mbut = ev.mouse_button;
  idle_ticks=0;
}

int event_waiting()
{ return wm->IsPending(); }


int demo_manager::start_recording(char *filename)
{
  if (!current_level) return 0;

  record_file=open_file(filename,"wb");
  if (record_file->open_failure()) { delete record_file; return 0; }

  char name[100];
  strcpy(name,current_level->name());

  the_game->load_level(name);
  record_file->write((void *)"DEMO,VERSION:2",14);
  record_file->write_uint8(strlen(name)+1);
  record_file->write(name,strlen(name)+1);


  if (DEFINEDP(symbol_value(l_difficulty)))
  {
    if (symbol_value(l_difficulty)==l_easy) record_file->write_uint8(0);
    else if (symbol_value(l_difficulty)==l_medium) record_file->write_uint8(1);
    else if (symbol_value(l_difficulty)==l_hard) record_file->write_uint8(2);
    else record_file->write_uint8(3);
  } else record_file->write_uint8(3);


  state=RECORDING;

  reset_game();

  return 1;
}

void demo_manager::do_inputs()
{
  switch (state)
  {
    case RECORDING :
    {
      base->packet.packet_reset();       // reset input buffer
      view *p=player_list;               // get current inputs
      for (; p; p=p->next)
        if (p->local_player())
          p->get_input();

      base->packet.write_uint8(SCMD_SYNC);
      base->packet.write_uint16(make_sync());
      demo_man.save_packet(base->packet.packet_data(),base->packet.packet_size());
      process_packet_commands(base->packet.packet_data(),base->packet.packet_size());

    } break;
    case PLAYING :
    {
      uint8_t buf[1500];
      int size;
      if (get_packet(buf,size))              // get starting inputs
      {
        process_packet_commands(buf, size);
        ivec2 mouse = the_game->GameToMouse(ivec2(player_list->pointer_x,
                                                  player_list->pointer_y),
                                            player_list);
        wm->SetMousePos((small_render ? 2 : 1) * mouse);
      }
      else
      {
    set_state(NORMAL);
    return ;
      }
    } break;
    default :
      break;
  }
}

void demo_manager::reset_game()
{
  if (dev&EDIT_MODE) toggle_edit_mode();
  the_game->set_state(RUN_STATE);
  rand_on=0;

  view *v=player_list;
  for (; v; v=v->next) { if (v->m_focus) v->reset_player(); }

  last_demo_mpos = ivec2(0, 0);
  last_demo_mbut = 0;
  current_level->set_tick_counter(0);

}

int demo_manager::start_playing(char *filename)
{
  uint8_t sig[15];
  record_file=open_file(filename,"rb");
  if (record_file->open_failure()) { delete record_file; return 0; }
  char name[100],nsize,diff;
  if (record_file->read(sig,14)!=14        ||
      memcmp(sig,"DEMO,VERSION:2",14)!=0   ||
      record_file->read(&nsize,1)!=1       ||
      record_file->read(name,nsize)!=nsize ||
      record_file->read(&diff,1)!=1)
  { delete record_file; return 0; }

  char tname[100],*c;
  strcpy(tname,name);
  c=tname;
  while (*c) { if (*c=='\\') *c='/'; c++; }

  bFILE *probe=open_file(tname,"rb");   // see if the level still exists?
  if (probe->open_failure()) { delete record_file; delete probe; return 0; }
  delete probe;

  the_game->load_level(tname);
  initial_difficulty = l_difficulty;

  switch (diff)
  {
    case 0: l_difficulty->SetValue(l_easy); break;
    case 1: l_difficulty->SetValue(l_medium); break;
    case 2: l_difficulty->SetValue(l_hard); break;
    case 3: l_difficulty->SetValue(l_extreme); break;
  }

  state=PLAYING;
  reset_game();



  return 1;
}

int demo_manager::set_state(demo_state new_state, char *filename)
{
  if (new_state==state) return 1;

  switch (state)
  {
    case RECORDING :
    { delete record_file; } break;
    case PLAYING :
    {
/*
      fade_in(cache.img(cache.reg("art/help.spe","sell6",SPEC_IMAGE,1)),8);
      Timer now; now.WaitMs(2000);
      fade_out(8);
*/
      delete record_file;
      l_difficulty = initial_difficulty;
      the_game->set_state(MENU_STATE);
      wm->PushUIEvent(ID_NULL, NULL);

      view *v=player_list;
      for (; v; v=v->next)  // reset all the players
      { if (v->m_focus) { v->reset_player(); v->m_focus->set_aistate(0); } }
      delete current_level;
      current_level=NULL;
      the_game->reset_keymap();
      base->input_state=INPUT_PROCESSING;


    } break;
    default :
      break;
  }

  switch (new_state)
  {
    case RECORDING :
    { return start_recording(filename); } break;
    case PLAYING :
    { return start_playing(filename); } break;
    case NORMAL :
    { state=NORMAL; } break;
  }

  return 1;
}

int demo_manager::save_packet(void *packet, int packet_size)   // returns non 0 if actually saved
{
  if (state==RECORDING)
  {
    uint16_t ps=lstl(packet_size);
    if (record_file->write(&ps,2)!=2 ||
    record_file->write(packet,packet_size)!=packet_size)
    {
      set_state(NORMAL);
      return 0;
    }
    return 1;
  } else return 0;
}

int demo_manager::get_packet(void *packet, int &packet_size)   // returns non 0 if actually loaded
{
  if (state==PLAYING)
  {
    uint16_t ps;
    if (record_file->read(&ps,2)!=2)
    {
      set_state(NORMAL);
      return 0;
    }
    ps=lstl(ps);

    if (record_file->read(packet,ps)!=ps)
    {
      set_state(NORMAL);
      return 0;
    }

    packet_size=ps;
    return 1;
  }
  return 0;
}

