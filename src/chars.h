/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __CHARACTERZ_HPP_
#define __CHARACTERZ_HPP_

#include <string.h>

#include "seq.h"
//#include "sound.h"
#include "ability.h"
#include "sdlport/event.h"
#include <stdarg.h>
#include <time.h>


enum character_state
{
    dead,
    dieing,
    stopped,
    start_run_jump, run_jump, run_jump_fall, end_run_jump,
    flinch_up, flinch_down,
    morph_pose,
    running
};

#define MAX_STATE (running+1)
extern char const *state_names[];

class named_field
{
  public :
  char *real_name;
  char *descript_name;
  named_field(char *real, char *fake)
  { real_name = strdup(real);
    descript_name = strdup(fake);
  }
  ~named_field() { free(real_name); free(descript_name); }
} ;



// all cflags default is 0
#define TOTAL_CFLAGS 11
enum { CFLAG_HURT_ALL,            // if object hurts all characters, not just player
       CFLAG_IS_WEAPON,           // if object is a collectable weapon (should have a logo)
       CFLAG_STOPPABLE,           // if object can be stopped by any other object
       CFLAG_CAN_BLOCK,           // if object can block other object
       CFLAG_HURTABLE,
       CFLAG_PUSHABLE,            // can push other pushable characters
       CFLAG_UNLISTABLE,          // if object should appear in object list during edit mode
       CFLAG_ADD_FRONT,
       CFLAG_CACHED_IN,
       CFLAG_NEED_CACHE_IN,
       CFLAG_UNACTIVE_SHIELD      // if object is not active (i.e. link 0 aistate==0)
                                  // then objects will not draw a damage when hitting it
     };
extern char const *cflag_names[TOTAL_CFLAGS];

// all object functions default to NULL
#define TOTAL_OFUNS 11
enum { OFUN_AI,                   // objects ai function called by the mover, should call (tick)
       OFUN_MOVER,                // objects move function, gets x y and but
       OFUN_DRAW,
       OFUN_MAP_DRAW,
       OFUN_DAMAGE,               // called when the object receives damage
       OFUN_NEXT_STATE,           // called at the end of an object sequence
       OFUN_USER_FUN,             // can by called (user_fun x y z)
       OFUN_CONSTRUCTOR,          // called when object is created, dev & play modes
       OFUN_RELOAD,               // called when the object is loaded from disk (not save games)
       OFUN_GET_CACHE_LIST,       // called on level load, should return list (a . b) a is character id, and b is other ids
       OFUN_CHANGE_TYPE
     } ;
extern char const *ofun_names[TOTAL_OFUNS];


class CharacterType
{
public:
    CharacterType(LList *args, LSymbol *name); // lisp object describes object
    ~CharacterType();

  uint16_t ts,tiv,tv; // total states, total index vars, total local vars
  sequence **seq;   // [0..ts-1]
  LSymbol **seq_syms;  // symbol describing what this state is [0..ts-1]

  LSymbol **vars;  // symbol describing variable names    [0..tiv-1]
  short *var_index; // index into local var                [0..tiv-1]

  void add_var(void *symbol, void *name);
  int add_state(LObject *symbol);           // returns index into seq to use
  int abil[TOTAL_ABILITIES];
  void *fun_table[TOTAL_OFUNS];             // pointers to lisp function for this object
  int logo,morph_mask,morph_power;
  long rangex,rangey,draw_rangex,draw_rangey;             // range off screen before character is skipped

  uint16_t cflags;
  void *get_fun(int name) { return fun_table[name]; }
  int get_cflag(int name) { return cflags&(1<<name); }
  void set_cflag(int name, int x) { if (x) cflags|=(1<<name);  else cflags&=~(1<<name); }
  int total_fields;                         // used by game editor to replace field names
  named_field **fields;

  sequence *get_sequence(character_state s);
  void add_sequence(character_state which, sequence *new_seq);
  int has_sequence(character_state s) { return s<ts && (seq[s]!=NULL); }
  int cache_in();    // returns false if out of cache memory
  void check_sizes();
  long isa_var_name(char *name);
} ;

extern CharacterType **figures;
int flinch_state(character_state state);

void *def_char(void *args);

extern int total_weapons;
extern int *weapon_types;    // maps 0..total_weapons into 'real' weapon type

#endif


















