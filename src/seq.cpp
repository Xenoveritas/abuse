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

#include "seq.h"
#include "lisp.h"

size_t sequence::MemUsage()
{
    size_t t = 0;
    for (int i = 0; i < total; i++)
        if (cache.loaded(seq[i]))
            t += cache.fig(seq[i])->MemUsage();
    return t;
}

int sequence::cache_in()
{
  int i;
  for (i=0; i<total; i++)
  {
    cache.note_need(seq[i]);
  }
  return 1;
}

sequence::sequence(char *filename, void *pict_list, void *advance_list)
{
  if (item_type(pict_list)==L_STRING)
    total=1;
  else
    total = ((LList *)pict_list)->GetLength();

  seq=(int *) malloc(sizeof(int)*total);
  if (item_type(pict_list)==L_STRING)
    seq[0]=cache.reg_object(filename,(LObject *)pict_list,SPEC_CHARACTER2,1);
  else
  {
    int i;
    for (i=0; i<total; i++)
    {
      seq[i]=cache.reg_object(filename,lcar(pict_list),SPEC_CHARACTER2,1);
      pict_list=lcdr(pict_list);
    }
  }
}

sequence::~sequence()
{
  free(seq);
}

/*sequence::sequence(char *filename, char *picts)
{
  char t[100],*s=picts,imname[100];
  int i,j;
  total=0;

  // first count the images
  while (token_type(s)!=sRIGHT_PAREN)
  {
    if (token_type(s)==sLEFT_PAREN)
    {
      get_token(s,t);    // left paren
      get_token(s,t);    // seq
      get_token(s,t);    // name
      i=get_number(s);
      total+=get_number(s)-i+1;
      get_token(s,t);    // right paren
    }
    else
    { get_token(s,t);
      total++;
    }
  }


  s=picts;
  seq=(int *) malloc(sizeof(int)*total);

  for (i=0; i<total; )
  {
    if (get_token(s,t)==sLEFT_PAREN)
    {
      get_token(s,t);      // left paren
      if (strcmp(t,"seq"))
      {
    printf("Expected seq at %s\n",s);
    exit(0);
      }
      get_token(s,t);
      int start,end;
      start=get_number(s);
      end=get_number(s);
      for (j=start; j<=end; j++)
      {
    snprintf(imname,100,"%s%04d.pcx",t,j);
    seq[i++]=cache.reg(filename,imname,SPEC_CHARACTER,1);
      }
      get_token(s,t);      // right paren
    }
    else
      seq[i++]=cache.reg(filename,t,SPEC_CHARACTER,1);
  }
}*/





