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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef WIN32
# include <Windows.h>
// Windows preprocessor magic shadows JWindowManager's CreateWindow function.
#undef CreateWindow
#endif

#include "common.h"

#include "game.h"

#include "view.h"
#include "lisp.h"
#include "jwindow.h"
#include "configuration.h"
#include "scroller.h"
#include "id.h"
#include "dev.h"
#include "jrand.h"
#include "dprint.h"
#include "transp.h"
#include "clisp.h"
#include "demo.h"
#include "sbar.h"
#include "nfserver.h"
#include "chat.h"

#define SHIFT_DOWN_DEFAULT 24
#define SHIFT_RIGHT_DEFAULT 0

extern int get_key_binding( char const *dir, int i );
view *player_list=NULL;
int morph_sel_frame_color;

view::~view()
{
    if (local_player())
        sbar.associate(NULL);

    if (total_weapons)
    {
        free(weapons);
        free(last_weapons);
    }
}


extern uint8_t bright_tint[256];

void view::add_ammo(int weapon_type, int total)
{
  if (weapon_type>=total_weapons || weapon_type<0)
  {
    printf("weapon out of range\n");
    return ;
  }
  if (weapons[weapon_type]==-1) return ;   // don't have weapon yet, can't give ammo

  weapons[weapon_type]+=total;
  if (weapons[weapon_type]<0)
    weapons[weapon_type]=0;

  if (weapons[weapon_type]>999)
    weapons[weapon_type]=999;

  if (weapon_total(current_weapon)==0 && current_weapon)
  {
    suggest.send_weapon_change=1;
    if (DEFINEDP(symbol_value(l_switch_to_powerful)) && symbol_value(l_switch_to_powerful))
    {
      int x=total_weapons-1;
      while (x>0 && (x==3 || weapons[x]<=0)) x--;
      suggest.new_weapon=x;
    } else
      suggest.new_weapon=0;
  }

}

void view::give_weapon(int type)
{
  if (type>=total_weapons || type<0)
  {
    printf("weapon out of range\n");
    return ;
  }
  if (weapons[type]==-1)
  {
    weapons[type]=0;
    sbar.need_refresh();
  }
}

int view::weapon_total(int type)
{
  if (type>=total_weapons || type<0)
  {
    printf("weapon out of range\n");
    return 0;
  }
  if (god) return 100;
  else if (weapons[type]==-1) return 0;
  else return weapons[type];
}


int32_t view::xoff()
{
    if (!m_focus)
        return pan_x;

    return Max(0, m_lastpos.x - (m_bb.x - m_aa.x + 1) / 2 + m_shift.x + pan_x);
}

int32_t view::interpolated_xoff()
{
    if (!m_focus)
        return pan_x;

    return Max(0, (m_lastlastpos.x + m_lastpos.x) / 2
                    - (m_bb.x - m_aa.x + 1) / 2 + m_shift.x + pan_x);
}

int32_t view::yoff()
{
    if (!m_focus)
        return pan_y;

    return Max(0, m_lastpos.y - (m_bb.y - m_aa.y + 1) / 2 - m_shift.y + pan_y);
}

int32_t view::interpolated_yoff()
{
    if (!m_focus)
        return pan_y;

    return Max(0, (m_lastlastpos.y + m_lastpos.y) / 2
                    - (m_bb.y - m_aa.y + 1) / 2 - m_shift.y + pan_y);
}


void view::update_scroll()
{
    if (!m_focus)
        return;

    m_lastlastpos = m_lastpos;

    if (m_focus->x > m_lastpos.x)
        m_lastpos.x = Max(m_lastpos.x, m_focus->x - no_xright);
    else if (m_focus->x < m_lastpos.x)
        m_lastpos.x = Min(m_lastpos.x, m_focus->x + no_xleft);

    if (m_focus->y > m_lastpos.y)
        m_lastpos.y = Max(m_lastpos.y, m_focus->y - no_ybottom);
    else if (m_focus->y < m_lastpos.y)
        m_lastpos.y = Min(m_lastpos.y, m_focus->y + no_ytop);
}

static char cur_user_name[20] = { 0 };

char const *get_login()
{
    if (cur_user_name[0])
        return cur_user_name;

#if defined __CELLOS_LV2__
    /* FIXME: retrieve login name */
    return "Player";
#elif defined WIN32
	DWORD bufferSize = 120;
	TCHAR *login;
	login = (TCHAR*) malloc(bufferSize * sizeof(TCHAR));
	if (GetUserName(login, &bufferSize))
	{
		return (char*) login;
	} else
	{
		return "unknown";
	}
#else
    char const *login = getlogin();
    return login ? login : "unknown";
#endif
}

void set_login(char const *name)
{
    strncpy(cur_user_name, name, 20);
}

view::view(game_object *focus, view *Next, int number)
{
    m_chat_buf[0] = 0;

  draw_solid=-1;
  no_xleft=0;
  no_xright=0;
  no_ytop=0;
  no_ybottom=0;
    m_lastlastpos = m_lastpos = focus ? ivec2(focus->x, focus->y) : ivec2(0);
  last_hp=last_ammo=-1;
  last_type=-1;
  tsecrets=secrets=0;
  tkills=kills=0;

  reset_keymap();

  ambient=32;
  current_weapon=0;

  strcpy(name,get_login());
  suggest.send_view=0;
  suggest.send_weapon_change=0;


  god=0;

  player_number=number;
    m_aa = ivec2(0);
    m_bb = ivec2(100);
    m_focus = focus;
  next=Next;
    m_shift = ivec2(SHIFT_RIGHT_DEFAULT, SHIFT_DOWN_DEFAULT);
  x_suggestion=0;
  y_suggestion=0;
  b1_suggestion=0;
  b2_suggestion=0;
  b3_suggestion=0;
  b4_suggestion=0;
  pointer_x=0;
  pointer_y=0;

  pan_x=0;
  pan_y=0;
  last_type=0;
  freeze_time=0;

  if (total_weapons)
  {
    weapons=(int32_t *)malloc(total_weapons*sizeof(int32_t));
    last_weapons=(int32_t *)malloc(total_weapons*sizeof(int32_t));
    memset(weapons,0xff,total_weapons*sizeof(int32_t));   // set all to -1
    memset(last_weapons,0xff,total_weapons*sizeof(int32_t));   // set all to -1
  }

  if (total_weapons)
    weapons[0]=0;
  if (local_player())
    sbar.associate(this);
  set_tint(number);
  set_team(-1);
  sbar.need_refresh();
}

int32_t view::x_center()
{
    return m_focus ? m_focus->x : (m_aa.x + m_bb.x) / 2;
}

int32_t view::y_center()
{
    return m_focus ? m_focus->y : (m_aa.y + m_bb.y) / 2;
}

void view::draw_character_damage()
{
  if (m_focus && drawable())
  {
    if (last_hp!=m_focus->hp()) draw_hp();
    int i;
    for (i=0; i<total_weapons; i++)
      if (weapons[i]!=last_weapons[i])
      {
    last_weapons[i]=weapons[i];
        sbar.draw_ammo(main_screen,i,weapons[i],current_weapon==i);
      }
  }
}



uint16_t make_sync()
{
  uint16_t x=0;
  if (!current_level) return 0;
  if (current_level)
  {
    view *f=player_list;
    for (; f; f=f->next)
    {
      if (f->m_focus)
      {
    x^=(f->m_focus->x&0xffff);
    x^=(f->m_focus->y&0xffff);
      }
    }
  }
  x^=rand_on;

  return x;
}



void view::get_input()
{
    int sug_x,sug_y,sug_b1,sug_b2,sug_b3,sug_b4;
    ivec2 sug_p(0, 0);

// NOTE:(AK) I have commented this out so we don't use the lisp
//        file "input.lsp" to get our key mappings.
/*    if( DEFINEDP( symbol_function( l_get_local_input ) ) )
    {
        void *ret = ((LSymbol *)l_get_local_input->EvalFunction(NULL);
        sug_x = lnumber_value( CAR( ret ) );
        ret = CDR( ret );
        sug_y = lnumber_value( CAR( ret ) );
        ret = CDR( ret );
        if( CAR( ret ) )
            sug_b1 = 1;
        else
            sug_b1 = 0;
        ret = CDR( ret );
        if( CAR( ret ) )
            sug_b2 = 1;
        else
            sug_b2 = 0;
        ret = CDR( ret );
        int x = lnumber_value( CAR( ret ) );
        ret = CDR( ret );
        if( x < 0 )
            sug_b3 = 1;
        else
            sug_b3 = 0;
        if( x > 0 )
            sug_b4 = 1;
        else sug_b4 = 0;

        int32_t bx = lnumber_value( CAR( ret ) );
        ret = CDR( ret );
        int32_t by = lnumber_value( CAR( ret ) );
        ret = CDR( ret );
        the_game->mouse_to_game( bx, by, sug_px, sug_py, this );

    }
    else*/
    {
        get_movement( 0, sug_x, sug_y, sug_b1, sug_b2, sug_b3, sug_b4 );
        if( m_focus )
        {
            sug_p = the_game->MouseToGame(last_demo_mpos);
            if(last_demo_mbut & 1)
                sug_b2 = 1;
            if(last_demo_mbut & 2)
                sug_b1 = 1;
        }
    }

    if( view_changed() )
    {
        base->packet.write_uint8( SCMD_VIEW_RESIZE );
        base->packet.write_uint8( player_number );
        base->packet.write_uint32( suggest.cx1 );
        base->packet.write_uint32( suggest.cy1 );
        base->packet.write_uint32( suggest.cx2 );
        base->packet.write_uint32( suggest.cy2 );

        base->packet.write_uint32( suggest.pan_x );
        base->packet.write_uint32( suggest.pan_y );
        base->packet.write_uint32( suggest.shift.y );
        base->packet.write_uint32( suggest.shift.x );
    }

    if( weapon_changed() )
    {
        base->packet.write_uint8( SCMD_WEAPON_CHANGE );
        base->packet.write_uint8( player_number );
        base->packet.write_uint32( suggest.new_weapon );
    }

    base->packet.write_uint8( SCMD_SET_INPUT );
    base->packet.write_uint8( player_number );

    uint8_t mflags = 0;
    if( sug_x > 0 )
        mflags |= 1;
    else if ( sug_x < 0 )
        mflags |= 2;

    if( sug_y > 0 )
        mflags |= 4;
    else if( sug_y < 0 )
        mflags |= 8;

    if( sug_b1 )
        mflags |= 16;
    if( sug_b2 )
        mflags |= 32;
    if( sug_b3 )
        mflags |= 64;
    if( sug_b4 )
        mflags |= 128;

    base->packet.write_uint8(mflags);
    base->packet.write_uint16((uint16_t)sug_p.x);
    base->packet.write_uint16((uint16_t)sug_p.y);
}


void view::add_chat_key(int key)  // return string if buf is complete
{
    int len = strlen(m_chat_buf);
  if (key==SDLK_BACKSPACE)
  {
    if (len)
    {
      m_chat_buf[len-1]=0;
      if (local_player() && chat)
        chat->draw_user(m_chat_buf);
    }
  } else if (key!=SDLK_RETURN)
  {
    m_chat_buf[len]=key;
    m_chat_buf[len+1]=0;
    if (local_player() && chat)
      chat->draw_user(m_chat_buf);
  }

  if (len>38 || key==SDLK_RETURN)
  {
    if (DEFINEDP(l_chat_input->GetFunction()))
    {
      game_object *o=current_object;
      current_object=m_focus;

      void *m = LSpace::Tmp.Mark();
      void *list=NULL;
      push_onto_list(LString::Create(m_chat_buf),list);
      ((LSymbol *)l_chat_input)->EvalFunction(list);
      LSpace::Tmp.Restore(m);

      current_object=o;

    } else
    {
      if (chat)
        chat->put_all(m_chat_buf);
    }
    m_chat_buf[0]=0;
    if (local_player() && chat)
      chat->draw_user(m_chat_buf);
  }
}

int view::process_input(char cmd, uint8_t *&pk)   // return 0 if something went wrong
{
  switch (cmd)
  {
    case SCMD_CHAT_KEYPRESS :
    {
      add_chat_key(*(pk++));
    } break;
    case SCMD_VIEW_RESIZE :
    {
      int32_t x[8];
      memcpy(x,pk,8*4);  pk+=8*4;
      m_aa = ivec2(lltl(x[0]), lltl(x[1]));
      m_bb = ivec2(lltl(x[2]), lltl(x[3]));

      pan_x=lltl(x[4]);
      pan_y=lltl(x[5]);
      m_shift = ivec2(lltl(x[7]), lltl(x[6]));
      if (small_render)
          small_render->Scale(m_bb - m_aa + ivec2(1));

      suggest.send_view=0;
      if (local_player())
        the_game->draw();
      return 1;
    }
    case SCMD_WEAPON_CHANGE :
    {
      int32_t x;
      memcpy(&x,pk,4);  pk+=4;
      current_weapon=lltl(x);

      if (local_player())
        sbar.need_refresh();
      suggest.send_weapon_change=0;
      return 1;
    } break;

    case SCMD_SET_INPUT :
    {
      uint8_t x=*(pk++);

      if (x&1) x_suggestion=1;
      else if (x&2) x_suggestion=-1;
      else x_suggestion=0;

      if (x&4) y_suggestion=1;
      else if (x&8) y_suggestion=-1;
      else y_suggestion=0;

      if (x&16) b1_suggestion=1; else b1_suggestion=0;
      if (x&32) b2_suggestion=1; else b2_suggestion=0;
      if (x&64) b3_suggestion=1; else b3_suggestion=0;
      if (x&128) b4_suggestion=1; else b4_suggestion=0;

      uint16_t p[2];
      memcpy(p,pk,2*2);  pk+=2*2;

      pointer_x=(int16_t)(lstl(p[0]));
      pointer_y=(int16_t)(lstl(p[1]));

      return 1;
    } break;
    case SCMD_KEYPRESS : set_key_down(*(pk++),1); break;
    case SCMD_EXT_KEYPRESS : set_key_down(*(pk++)+256,1); break;
    case SCMD_KEYRELEASE : set_key_down(*(pk++),0); break;
    case SCMD_EXT_KEYRELEASE : set_key_down(*(pk++)+256,0); break;
  }
  return 1;
}

int view::local_player()
{
  return player_number==client_number();
}

void view::next_weapon()
{
  int c=current_weapon;

  while (c<total_weapons-1)
  {
    c++;
    if (weapon_total(c)>0)
    {
      suggest.send_weapon_change=1;
      suggest.new_weapon=c;
      return ;
    }
  }

  c=0;
  while (c!=current_weapon)
  {
    if (weapon_total(c)>0)
    {
      suggest.send_weapon_change=1;
      suggest.new_weapon=c;
      return ;
    }
    c++;
  }
}

void view::last_weapon()
{

  int c=current_weapon;

  while (c>=1)
  {
    c--;
    if (weapon_total(c)>0 || c==0)
    {
      suggest.send_weapon_change=1;
      suggest.new_weapon=c;
      return ;
    }
  }

  c=total_weapons-1;
  while (c!=current_weapon)
  {
    if (weapon_total(c)>0 || c==0)
    {
      suggest.send_weapon_change=1;
      suggest.new_weapon=c;
      return ;
    }
    c--;
  }

}

int view::handle_event(SDL_Event &ev)
{
	// FIXME: I think this is where the bindings will be injected
    if( ev.type == SDL_KEYDOWN )
    {
        if( ev.key.keysym.sym == SDLK_COMMA )
        {
            if( total_weapons )
            {
                last_weapon();
            }
            return 1;
        }
        else if( ev.key.keysym.sym == SDLK_PERIOD )
        {
            if( total_weapons )
            {
                next_weapon();
            }
            return 1;
        }
        else if( ev.key.keysym.sym == get_key_binding( "b3", 0 ) )
        {
            if( total_weapons )
            {
                last_weapon();
            }
            return 1;
        }
		else if (ev.key.keysym.sym == get_key_binding("b4", 0))
        {
            if( total_weapons )
            {
                next_weapon();
            }
            return 1;
        }

		switch (ev.key.keysym.sym)
        {
            case SDLK_1:
			case SDLK_2:
			case SDLK_3:
			case SDLK_4:
			case SDLK_5:
			case SDLK_6:
			case SDLK_7:
            {
                if((( dev & EDIT_MODE ) == 0 ) && ( weapon_total( ev.key.keysym.sym - SDLK_1 ) > 0 ))
                {
                    suggest.send_weapon_change = 1;
					suggest.new_weapon = ev.key.keysym.sym - SDLK_1;
                }
            } break;

            case SDLK_HOME:
            case SDLK_LCTRL:
            case SDLK_RCTRL:
            {
                if( total_weapons )
                {
                    last_weapon();
                }
                return 1;
            } break;
            case SDLK_PAGEUP:
            case SDLK_INSERT:
            {
                if( total_weapons )
                {
                    next_weapon();
                }
                return 1;
            } break;
        }
    }
    return 0;
}

void view::draw_hp()
{
    if (m_focus)
    {
        int h = m_focus->hp();
        last_hp=h;
        sbar.draw_health( main_screen, m_focus->hp() );
    }
    else
    {
        sbar.draw_health( main_screen, 0 );
    }
}

int view::drawable()
{
    return local_player();
}


void recalc_local_view_space()   // calculates view areas for local players, should be called
                                 // when adding or deleting local players
{
  if (main_screen)
  {
    int t=total_local_players();
    if (!t) return ;

    int Xres=small_render ? xres/2 : xres;
    int Yres=small_render ? yres/2 : yres;

    int h=Yres/t;
    int w=h*320/200,y=5;
    if (w<300) w=300;

    for (view *f=player_list; f; f=f->next)
    {
      if (f->local_player())
      {
    f->suggest.cx1=Xres/2-w/2;
    f->suggest.cx2=Xres/2+w/2;
    if (f->suggest.cx1<2) f->suggest.cx1=2;
    if (f->suggest.cx2>Xres-2) f->suggest.cx2=Xres-2;

    f->suggest.cy1=y;
    f->suggest.cy2=h-(total_weapons ? 33 : 0);

    f->suggest.shift = f->m_shift;
    f->suggest.pan_x=f->pan_x;
    f->suggest.pan_y=f->pan_y;
    f->suggest.send_view=1;

    if (!player_list->next)
    {
      f->m_aa = ivec2(f->suggest.cx1, f->suggest.cy1);
      f->m_bb = ivec2(f->suggest.cx2, f->suggest.cy2);
      f->suggest.send_view = 0;
    }
    y+=h;
      }
    }
  }

}


void set_local_players(int total)
{
  int rdw=0;
  if (total<1) return ;

  view *last=NULL;
  for (view *f=player_list; f; f=f->next)
  {
    if (total && f->local_player())
      total--;
    else if (!total && f->local_player())  // too many local players, delete this one
    {
      view *n=last->next;
      while (n && !n->local_player()) n=n->next;  // find next local player

      if (last)
        last->next=n;
      else
      {
    if (n)    // make sure we have at least one local player
          player_list=n;
      }
      last=f;
      rdw=1;
    }
  }

  while (total)   // see if we need to add new players
  {
    game_object *o=create(current_start_type,50,50);
    view *v;
    if (!player_list)
    {
      player_list=new view(o,NULL,0);
      v=player_list;
    }
    else
    {
      view *f=player_list;
      for (; f && f->next; f=f->next);
      f->next=new view(o,NULL,f->player_number+1);
      v=f->next;
    }
    v->m_aa = ivec2(320 / 2 - 155, 200 / 2 - 95);
    v->m_bb = ivec2(320 / 2 + 155, 200 / 2 + total_weapons ? 60 : 95);
    v->m_focus->set_controller(v);
    total--;
    rdw=1;
  }
  if (rdw)
    recalc_local_view_space();
}


int total_local_players()
{
  int t=0;
  for (view *f=player_list; f; f=f->next)
    if (f->local_player()) t++;
  return t;
}

void view::set_input(int cx, int cy, int b1, int b2, int b3, int b4, int px, int py)
{
    x_suggestion=cx;
    y_suggestion=cy;
    b1_suggestion=b1;
    b2_suggestion=b2;
    b3_suggestion=b3;
    b4_suggestion=b4;
    pointer_x=px;
    pointer_y=py;
}



void view::reset_player()
{
  if (m_focus)
  {

    game_object *start=current_level ? current_level->get_random_start(320,m_focus->controller()) : 0;
    m_focus->defaults();
    if (start)
    {
      m_focus->x=start->x;
      m_focus->y=start->y;
      dprintf("reset player position to %d %d\n",start->x,start->y);
    }
    m_focus->set_state(stopped);
    m_focus->set_tint(_tint);
    m_focus->set_team(_team);
    memset(weapons,0xff,total_weapons*sizeof(int32_t));
    memset(last_weapons,0xff,total_weapons*sizeof(int32_t));

    m_shift = ivec2(SHIFT_RIGHT_DEFAULT, SHIFT_DOWN_DEFAULT);

    if (total_weapons)
      weapons[0]=0;  // give him the first weapon
    current_weapon=0;

    memset(m_focus->lvars,0,figures[m_focus->otype]->tv*4);
    m_focus->set_aistate(0);
    if (figures[m_focus->otype]->get_fun(OFUN_CONSTRUCTOR))
    {
      game_object *o=current_object;
      current_object=m_focus;
      ((LSymbol *)figures[m_focus->otype]->get_fun(OFUN_CONSTRUCTOR))->EvalUserFunction(NULL);
      current_object=o;
    }
    sbar.redraw(main_screen);

    int i;
    for (i=0; i<m_focus->total_objects(); i++)   // reset the vars for the attached objects
    {
      game_object *o=m_focus->get_object(i);
      memset(o->lvars,0,figures[o->otype]->tv*4);
    }

  }
}





object_node *make_player_onodes(int player_num)
{
  object_node *first=NULL,*last=NULL;
  for (view *o=player_list; o; o=o->next)
  {
    if (o->m_focus && (player_num==-1 || o->player_number==player_num))
    {
      if (!object_to_number_in_list(o->m_focus,first))
      {
    object_node *q=new object_node(o->m_focus,NULL);
    if (first)
      last->next=q;
    else first=q;
    last=q;
      }
      for (int i=0; i<o->m_focus->total_objects(); i++)
      {
    game_object *p=o->m_focus->get_object(i);

    if (!object_to_number_in_list(p,first))
    {
      object_node *q=new object_node(p,NULL);
      if (first)
        last->next=q;
      else first=q;
      last=q;
    }
      }
    }
  }
  return first;
}




enum { V_CX1, V_CY1, V_CX2, V_CY2,
       V_SHIFT_DOWN, V_SHIFT_RIGHT,
       V_GOD,
       V_PLAYER_NUMBER,
       V_DRAW_SOLID,
       V_LIVES,
       V_CURRENT_WEAPON,
       V_X_SUGGESTION, V_Y_SUGGESTION, V_B1_SUGGESTION, V_B2_SUGGESTION, V_B3_SUGGESTION, V_B4_SUGGESTION,
       V_PAN_X, V_PAN_Y,
       V_NO_XLEFT, V_NO_XRIGHT, V_NO_YTOP, V_NO_YBOTTOM,
       V_LAST_X, V_LAST_Y, V_LAST_LEFT, V_LAST_RIGHT, V_LAST_UP, V_LAST_DOWN,
       V_LAST_B1, V_LAST_B2, V_LAST_B3, V_LAST_B4,
       V_LAST_HP,
       V_SECRETS, V_KILLS, V_TSECRETS, V_TKILLS,
       V_AMBIENT,
       V_POINTER_X, V_POINTER_Y,
       V_LAST_LAST_X, V_LAST_LAST_Y,
       V_FREEZE_TIME };

#define TVV (V_FREEZE_TIME+1)

static char const *vv_names[TVV] =
{
    "view.cx1",  "view.cy1",  "view.cx2",  "view.cy2",
    "view.shift_down",  "view.shift_right",
    "view.god",
    "view.player_number",
    "view.draw_solid",
    "view.lives",
    "view.current_weapon",
    "view.x_suggestion",  "view.y_suggestion",
    "view.b1_suggestion",  "view.b2_suggestion",  "view.b3_suggestion",  "view.b4_suggestion",
    "view.pan_x",  "view.pan_y",
    "view.no_xleft",  "view.no_xright",  "view.no_ytop",  "view.no_ybottom",
    "view.last_x",  "view.last_y",  "view.last_left",  "view.last_right",  "view.last_up",  "view.last_down",
    "view.last_b1",  "view.last_b2",  "view.last_b3",  "view.last_b4",
    "view.last_hp",
    "view.secrets",  "view.kills",  "view.tsecrets",  "view.tkills",
    "view.ambient",
    "view.pointer_x",  "view.pointer_y",
    "view.last_last_x",  "view.last_last_y",
    "view.freeze_time"
};


int total_view_vars()
{ return TVV;
}

char const *get_view_var_name(int num)
{ return vv_names[num]; }

int32_t view::get_view_var_value(int num)
{
  switch (num)
  {
    case V_CX1 : return m_aa.x; break;
    case V_CY1 : return m_aa.y; break;
    case V_CX2 : return m_bb.x; break;
    case V_CY2 : return m_bb.y; break;
    case V_SHIFT_DOWN : return m_shift.y; break;
    case V_SHIFT_RIGHT : return m_shift.x; break;
    case V_GOD : return god; break;
    case V_PLAYER_NUMBER : return player_number; break;

    case V_DRAW_SOLID : return draw_solid; break;
    case V_CURRENT_WEAPON : return current_weapon; break;
    case V_X_SUGGESTION : return x_suggestion; break;
    case V_Y_SUGGESTION : return y_suggestion; break;
    case V_B1_SUGGESTION : return b1_suggestion; break;
    case V_B2_SUGGESTION : return b2_suggestion; break;
    case V_B3_SUGGESTION : return b3_suggestion; break;
    case V_B4_SUGGESTION : return b4_suggestion; break;

    case V_PAN_X : return pan_x; break;
    case V_PAN_Y : return pan_y; break;
    case V_NO_XLEFT : return no_xleft; break;
    case V_NO_XRIGHT : return no_xright; break;
    case V_NO_YTOP : return no_ytop; break;
    case V_NO_YBOTTOM : return no_ybottom; break;
    case V_LAST_X : return m_lastpos.x; break;
    case V_LAST_Y : return m_lastpos.y; break;
    case V_LAST_LEFT : return last_left; break;
    case V_LAST_RIGHT : return last_right; break;
    case V_LAST_UP : return last_up; break;
    case V_LAST_DOWN : return last_down; break;
    case V_LAST_B1 : return last_b1; break;
    case V_LAST_B2 : return last_b2; break;
    case V_LAST_B3 : return last_b3; break;
    case V_LAST_B4 : return last_b4; break;
    case V_LAST_HP : return last_hp; break;
    case V_SECRETS : return secrets; break;
    case V_KILLS : return kills; break;
    case V_TSECRETS : return tsecrets; break;
    case V_TKILLS : return tkills; break;
    case V_AMBIENT : return ambient; break;
    case V_POINTER_X : return pointer_x; break;
    case V_POINTER_Y : return pointer_y; break;
    case V_LAST_LAST_X : return m_lastlastpos.x; break;
    case V_LAST_LAST_Y : return m_lastlastpos.y; break;
    case V_FREEZE_TIME : return freeze_time; break;
  }
  return 0;
}



int32_t view::set_view_var_value(int num, int32_t x)
{
  switch (num)
  {
    case V_CX1 : m_aa.x = x; break;
    case V_CY1 : m_aa.y = x; break;
    case V_CX2 : m_bb.x = x; break;
    case V_CY2 : m_bb.y = x; break;
    case V_SHIFT_DOWN : m_shift.y = x; break;
    case V_SHIFT_RIGHT : m_shift.x = x; break;
    case V_GOD : god=x; break;
    case V_PLAYER_NUMBER : { player_number=x; if (local_player()) sbar.associate(this); }  break;

    case V_DRAW_SOLID : draw_solid=x; break;
    case V_CURRENT_WEAPON : { current_weapon=x; sbar.need_refresh(); } break;
    case V_X_SUGGESTION : x_suggestion=x; break;
    case V_Y_SUGGESTION : y_suggestion=x; break;
    case V_B1_SUGGESTION : b1_suggestion=x; break;
    case V_B2_SUGGESTION : b2_suggestion=x; break;
    case V_B3_SUGGESTION : b3_suggestion=x; break;
    case V_B4_SUGGESTION : b4_suggestion=x; break;

    case V_PAN_X : pan_x=x; break;
    case V_PAN_Y : pan_y=x; break;
    case V_NO_XLEFT : no_xleft=x; break;
    case V_NO_XRIGHT : no_xright=x; break;
    case V_NO_YTOP : no_ytop=x; break;
    case V_NO_YBOTTOM : no_ybottom=x; break;
    case V_LAST_X : m_lastpos.x=x; break;
    case V_LAST_Y : m_lastpos.y=x; break;
    case V_LAST_LEFT : last_left=x; break;
    case V_LAST_RIGHT : last_right=x; break;
    case V_LAST_UP : last_up=x; break;
    case V_LAST_DOWN : last_down=x; break;
    case V_LAST_B1 : last_b1=x; break;
    case V_LAST_B2 : last_b2=x; break;
    case V_LAST_B3 : last_b3=x; break;
    case V_LAST_B4 : last_b4=x; break;

    case V_LAST_HP : last_hp=x; break;
    case V_SECRETS : secrets=x; break;
    case V_KILLS : kills=x; break;
    case V_TSECRETS : tsecrets=x; break;
    case V_TKILLS : tkills=x; break;
    case V_AMBIENT : ambient=x; break;
    case V_POINTER_X : pointer_x=x; break;
    case V_POINTER_Y : pointer_y=x; break;
    case V_LAST_LAST_X : m_lastlastpos.x = x; break;
    case V_LAST_LAST_Y : m_lastlastpos.y = x; break;
    case V_FREEZE_TIME : freeze_time=x; break;
  }
  return 1;
}


void view::configure_for_area(area_controller *a)
{
  if (a->ambient>=0 && a->ambient!=ambient)
  {
    if (ambient>a->ambient)
    {
      ambient-=a->ambient_speed;
      if (ambient<a->ambient)
        ambient=a->ambient;
    }
    else
    {
      ambient+=a->ambient_speed;
      if (ambient>a->ambient)
        ambient=a->ambient;
    }
  }

  if (!view_shift_disabled)
  {
    if (a->view_xoff!=pan_x)
    {
      if (pan_x>a->view_xoff)
      {
    pan_x-=a->view_xoff_speed;
    if (pan_x<a->view_xoff)
        pan_x=a->view_xoff;
      }
      else
      {
    pan_x+=a->view_xoff_speed;
    if (pan_x>a->view_xoff)
        pan_x=a->view_xoff;
      }
    }

    if (a->view_yoff!=pan_y)
    {
      if (pan_y>a->view_yoff)
      {
    pan_y-=a->view_yoff_speed;
    if (pan_y<a->view_yoff)
        pan_y=a->view_yoff;
      }
      else
      {
    pan_y+=a->view_yoff_speed;
    if (pan_y>a->view_yoff)
        pan_y=a->view_yoff;
      }
    }
  }
}


void process_packet_commands(uint8_t *pk, int size)
{
  int32_t sync_uint16=-1;

  if (!size) return ;
  pk[size]=SCMD_END_OF_PACKET;

  uint8_t cmd;
  int already_reloaded=0;


  do
  {
    cmd=*(pk++);
    switch (cmd)
    {
      case SCMD_WEAPON_CHANGE :
      case SCMD_SET_INPUT :
      case SCMD_VIEW_RESIZE :
      case SCMD_KEYPRESS :
      case SCMD_KEYRELEASE :
      case SCMD_EXT_KEYPRESS :
      case SCMD_EXT_KEYRELEASE :
      case SCMD_CHAT_KEYPRESS :
      {
    uint8_t player_num=*(pk++);

    view *v=player_list;
    for (; v && v->player_number!=player_num; v=v->next);
    if (v)
    {
      if (v->player_number==player_num)
      v->process_input(cmd,pk);
    }
    else
    {
      dprintf("Evil error : bad player number in packet\n");
      return ;
    }
      } break;
      case SCMD_RELOAD :
      {
    if (!already_reloaded)
    {
      net_reload();
      already_reloaded=1;
    }
      } break;

      case SCMD_SYNC :
      {
    uint16_t x;
    memcpy(&x,pk,2);  pk+=2;
    x=lstl(x);
    if (demo_man.current_state()==demo_manager::PLAYING)
    sync_uint16=make_sync();

    if (sync_uint16==-1)
    sync_uint16=x;
    else if (x!=sync_uint16 && !already_reloaded)
    {
      dprintf("out of sync %d (packet=%d, calced=%d)\n",current_level->tick_counter(),x,sync_uint16);
      if (demo_man.current_state()==demo_manager::NORMAL)
        net_reload();
      already_reloaded=1;
    }
      } break;
      case SCMD_DELETE_CLIENT :
      {
    uint8_t player_num=*(pk++);
    view *v=player_list,*last=NULL;
    for (; v && v->player_number!=player_num; v=v->next)
    last=v;
    if (!v)
    dprintf("evil : delete client %d, but no such client\n");
    else
    {

      // make a list of all objects associated with this player
      object_node *on=make_player_onodes(player_num);
      while (on)
      {
        current_level->delete_object(on->me);
        object_node *last=on;
        on=on->next;
        delete last;
      }

      v->m_focus=NULL;
      if (last)
      last->next=v->next;
      else player_list=player_list->next;

      delete v;
    }
      } break;
      default :
      dprintf("Unknown net command %d\n",cmd);

    }
  } while (cmd!=SCMD_END_OF_PACKET);
}

void view::set_tint(int tint)
{
    if(tint < 0)
        tint = 0;
    _tint = tint;
    m_focus->set_tint(tint);
}

int view::get_tint()
{
    return _tint;
}

void view::set_team(int team)
{
    if(team < 0)
        team = 0;
    _team = team;
    m_focus->set_team(team);
}

int view::get_team()
{
    return _team;
}

