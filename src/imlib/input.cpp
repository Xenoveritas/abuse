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

#include <string.h>

#include "common.h"

#include "input.h"

void button::remap(Filter *f)
{
  if (visual)
  {
    f->Apply(visual);
    if (pressed)
      f->Apply(pressed);
  }
}

void button_box::press_button(int id)      // if button box doesn't contain id, nothing happens
{
}

void button_box::remap(Filter *f)
{
  for (button *b=buttons; b; b=(button *)b->next)
    b->remap(f);
}

ifield *button_box::find(int search_id)
{
  if (search_id==id) return this;
  for (ifield *i=(ifield *)buttons; i; i=i->next)
    if (search_id==i->id) return i;
  return NULL;
}

button_box::button_box(int X, int Y, int ID, int MaxDown, button *Buttons, ifield *Next)
{
  m_pos = ivec2(X, Y); id=ID; next=Next;
  buttons=Buttons;
  maxdown=MaxDown;
  if (buttons && maxdown) buttons->push();  // the first button is automatically selected!
}

button_box::~button_box()
{
  while (buttons)
  {
    button *b=buttons;
    buttons=(button *)buttons->next;
    delete b;
  }
}

void button_box::area(int &x1, int &y1, int &x2, int &y2)
{
  button *b=buttons;
  if (!b) return ;
  else
  {
    b->area(x1,y1,x2,y2);
    int xp1,yp1,xp2,yp2;
    for (b=(button *)b->next; b; b=(button *)b->next)
    {
      b->area(xp1,yp1,xp2,yp2);
      if (xp1<x1) x1=xp1;
      if (xp2>x2) x2=xp2;
      if (yp1<y1) y1=yp1;
      if (yp2>y2) y2=yp2;
    }
  }
}

void button_box::draw_first(image *screen)
{
  for (button *b=buttons; b; b=(button *)b->next)
    b->draw_first(screen);
}

void button_box::draw(int active, image *screen)
{
  return ;
}

void button_box::Move(ivec2 pos)
{
    for(button * b = buttons; b; b = (button *)b->next)
        b->Move(pos + b->m_pos);
    m_pos = pos;
}

char *button_box::read()
{
  for (button *b=buttons; b; b=(button *)b->next)
  {
    if (*((int *)b->read())==0)
      return (char *)b;
  }
  return NULL;
}

void button_box::handle_event(SDL_Event &ev, image *screen, InputManager *im)
{
  switch (ev.type)
  {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    {
      int x1,y1,x2,y2;
      int found=0;
      for (button *b=buttons; !found && b; b=(button *)b->next)  // see if the user clicked on a button
      {
    b->area(x1,y1,x2,y2);
    if (ev.button.x>=x1 && ev.button.x<=x2 &&
        ev.button.y>=y1 && ev.button.y<=y2)
    {
      b->handle_event(ev,screen,im);

      int total=0;
      button *b2=buttons;
      for (; b2; b2=(button *)b2->next)
        if (*((int *)b2->read())==0)
          total++;

      if (*((int *)b->read())==0)  // did the user press or release the button
      {
        if (total>maxdown)
        {
          for (b2=buttons; total>maxdown && b2; b2=(button *)b2->next)
            if ((b!=b2 || maxdown==0) && *((int *)b2->read())==0)
        {
          total--;
          b2->push();
          b2->draw_first(screen);
        }
        }
        b->draw_first(screen);
      } else if (total==0 && maxdown)
        b->push();    // don't let the user de-press a button if non others are selected.

      found=1; // don't look at anymore buttons

    }
      }
    } break;
  }
}


void button_box::add_button(button *b)
{
  b->next=buttons;
  buttons=b;
}


void button_box::arrange_left_right()
{
    ivec2 on = m_pos;
    for (button *b = buttons; b; b = (button *)b->next)
    {
        int x1, y1, x2, y2;
        b->area(x1, y1, x2, y2);
        b->m_pos = on;
        on.x += (x2 - x1 + 1) + 1;
    }
}

void button_box::arrange_up_down()
{
    ivec2 on = m_pos;
    for (button *b = buttons; b; b = (button *)b->next)
    {
        int x1, y1, x2, y2;
        b->area(x1,y1,x2,y2);
        b->m_pos = on;
        on.y += (y2 - y1 + 1) + 1;
    }
}

void button::change_visual(image *new_visual)
{
  CHECK(visual);
  visual=new_visual;
}

void button::area(int &x1, int &y1, int &x2, int &y2)
{
    ivec2 pos1 = m_pos;
    ivec2 pos2 = m_pos;

    if (pressed)
        pos2 += pressed->Size() - ivec2(1);
    else if (text)
        pos2 += wm->font()->Size() * ivec2(strlen(text), 1) + ivec2(6);
    else
        pos2 += visual->Size() + ivec2(6);

    x1 = pos1.x; y1 = pos1.y;
    x2 = pos2.x; y2 = pos2.y;
}


button::button(int X, int Y, int ID, char const *Text, ifield *Next)
{
    m_pos = ivec2(X, Y);
    id = ID;
    act_id=-1;
    text = strdup(Text);
    up=1; next=Next; act=0;
    visual=NULL;
    pressed=NULL;
}


button::button(int X, int Y, int ID, image *vis, ifield *Next)
{
    m_pos = ivec2(X, Y);
    id=ID; text=NULL;
    act_id=-1;
    visual=vis; up=1; next=Next; act=0;
    pressed=NULL;
}

button::button(int X, int Y, int ID, image *Depressed, image *Pressed, image *active, ifield *Next)
{
    m_pos = ivec2(X, Y);
    id=ID; text=NULL;
    act_id=-1;
    visual=Depressed; up=1; next=Next; act=0;
    pressed=Pressed;
    act_pict=active;
}


void text_field::change_data(const char *new_data, int new_cursor, // cursor==-1, does not change it.
                 int active, image *screen)
{
  if (strlen(format)<strlen(new_data))
    data=(char *)realloc(data,strlen(new_data));

  strcpy(data,new_data);
  if (new_cursor!=-1)
    cur=new_cursor;
  draw_first(screen);
  draw(active,screen);
}

char *text_field::read()
{
  while (*data && data[strlen(data)-1]==' ') data[strlen(data)-1]=0;
  return data;
}

void text_field::handle_event(SDL_Event &ev, image *screen, InputManager *im)
{
    int xx;
    switch(ev.type)
    {
    case SDL_KEYDOWN:
        switch (ev.key.keysym.sym)
        {
        case SDLK_LEFT:
            if (cur)
            {
                draw_cur(wm->dark_color(),screen);
                cur--;
                draw_cur(wm->bright_color(),screen);
            }
            break;
        case SDLK_RIGHT:
            if (cur<(int)strlen(format)-1)
            {
                draw_cur(wm->dark_color(),screen);
                cur++;
                draw_cur(wm->bright_color(),screen);
            }
            break;
        case SDLK_END:
            if (cur!=last_spot())
            {
                draw_cur(wm->dark_color(),screen);
                cur=last_spot();
                if (cur==(int)strlen(format)-1)
                    cur--;
                draw_cur(wm->bright_color(),screen);
            }
            break;
        case SDLK_HOME:
            if (cur)
            {
                draw_cur(wm->dark_color(),screen);
                cur=0;
                draw_cur(wm->bright_color(),screen);
            }
            break;
        case SDLK_BACKSPACE:
            if (cur)
            {
                draw_cur(wm->dark_color(),screen);
                cur--;
                for (xx=cur; xx<(int)strlen(format)-1; xx++)
                    data[xx]=data[xx+1];
                data[strlen(format)-1]=' ';
                draw_text(screen);
                draw_cur(wm->bright_color(),screen);
                wm->PushUIEvent(id, this);
            }
            break;
        default:
            // FIXME: This should be using SDL_TEXTEDITING and SDL_TEXTINPUT
            // instead.
            if ((ev.key.keysym.mod == KMOD_NONE || (ev.key.keysym.mod & KMOD_SHIFT) != 0) && ev.key.keysym.sym >= ' ' && ev.key.keysym.sym <= '~')
            {
                char key = ev.key.keysym.sym;
                // See if SHIFT is down
                if ((ev.key.keysym.mod & KMOD_SHIFT) != 0)
                {
                    // If we were using SDL_TEXTINPUT like we should, this
                    // wouldn't matter. But for now, it does.
                    // FIXME: And, of course, this rather blatantly ignores
                    // keyboard mappings. Oh well.
                    if (key >= 'a' && key <= 'z')
                    {
                        key -= 32;
                    }
                    else if (key >= '1' && key <= '5')
                    {
                        key -= 16;
                    }
                    else
                    {
                        // FIXME
                    }
                }
                draw_cur(wm->dark_color(),screen);
                for (xx=strlen(format)-1; xx>cur && xx>0; xx--)
                    data[xx]=data[xx-1];
                data[cur]=key;
                if (cur<(int)strlen(format)-1)
                    cur++;
                data[strlen(format)]=0;
                draw_text(screen);
                draw_cur(wm->bright_color(),screen);
                wm->PushUIEvent(id, this);
            }
            break;
        }
        break;
    // FIXME: The following events won't be fired until SDL_StartTextInput()
    // and SDL_StopTextInput() are implemented on focus and blur.
    // Likewise SDL_SetTextInputRect() should be used.
    // New for SDL2 (and the primary reason to replace the custom event system
    // with SDL2's):
    case SDL_TEXTEDITING:
        // FIXME: This means we've received "non-commited" text. For example,
        // it's possible for a user to enter an accent and then a letter that
        // will be combined into a single character. We don't care for now in
        // any case because Abuse doesn't support anything besides ASCII.
        // Conceptually we should display this in some other color as its
        // entered. Also, again, it hardly matters.
        break;
    case SDL_TEXTINPUT:
        // This is the "commit" event where we take the text input and actually
        // store it into the text buffer.
        printf("Text Input: [%s]\n", ev.text.text);
        break;
    }
}

void text_field::draw(int active, image *screen)
{
  if (active)
  {
    screen->Rectangle(ivec2(xstart(), m_pos.y), ivec2(xend(), yend()),
                      wm->bright_color());
    draw_cur(wm->bright_color(),screen);
  }
  else
  {
    screen->Rectangle(ivec2(xstart(), m_pos.y), ivec2(xend(), yend()),
                      wm->dark_color());
    draw_cur(wm->dark_color(),screen);
  }
}

void text_field::area(int &x1, int &y1, int &x2, int &y2)
{
    x1 = m_pos.x; y1 = m_pos.y;
    x2 = xend(); y2 = yend();
}

text_field::text_field(int X, int Y, int ID, char const *Prompt,
                       char const *Format, char const *Data, ifield *Next)
{
    int slen=(strlen(Format)>strlen(Data) ? strlen(Format) : strlen(Data));

    m_pos = ivec2(X, Y);
    id=ID;
    prompt = strdup(Prompt);
    format=strcpy((char *)malloc(slen+1),Format);
    data=strcpy((char *)malloc(slen+1),Data);
    cur=strlen(data);
    while (cur && data[cur-1]==' ') cur--;
    next=Next;
}

text_field::text_field(int X, int Y, int ID, char const *Prompt,
                       char const *Format, double Data, ifield *Next)
{
  char num[20];
  sprintf(num,"%g",Data);
  int slen=(strlen(Format)>strlen(num) ? strlen(Format) : strlen(num));
  m_pos = ivec2(X, Y); id=ID;
  prompt = strdup(Prompt);
  format=strcpy((char *)malloc(slen+1),Format);
  data=strcpy((char *)malloc(slen+1),num);
  cur=strlen(num);
  while (cur && data[cur-1]==' ') cur--;
  next=Next;
}


void button::push()
{ up=!up; }

void button::handle_event(SDL_Event &ev, image *screen, InputManager *im)
{
    if ((ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_RETURN)
        || (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT))
    {
        int  x1,y1,x2,y2;
        area(x1,y1,x2,y2);
        up=!up;
        draw_first(screen);
        draw(act,screen);
        wm->PushUIEvent(id, this);
    }
}

void button::draw(int active, image *screen)
{
  int x1,y1,x2,y2,color=(active ? wm->bright_color() : wm->medium_color());
  area(x1,y1,x2,y2);
  if (active!=act  && act_id!=-1 && active)
    wm->PushUIEvent(act_id, NULL);

  if (pressed)
  {
    if (up)
    {
      if (!active)
        screen->PutImage(visual, m_pos);
      else
        screen->PutImage(pressed, m_pos);
    } else screen->PutImage(act_pict, m_pos);
  }
  else
  {
    screen->Rectangle(ivec2(x1 + 2, y1 + 2), ivec2(x2 - 2, y2 - 2), color);
    act = active;
  }
}

void button::draw_first(image *screen)
{
    if (pressed)
    {
        draw(0, screen);
        return;
    }

    int x1,y1,x2,y2;
    area(x1,y1,x2,y2);

    if (up)
    {
      screen->Rectangle(ivec2(x1, y1), ivec2(x2, y2), wm->black());
//      screen->widget_bar(,wm->bright_color(),wm->medium_color(),wm->dark_color());
      screen->WidgetBar(ivec2(x1 + 1, y1 + 1), ivec2(x2 - 1, y2 - 1),
                        wm->bright_color(),wm->medium_color(),wm->dark_color());
    }
    else
    {
      screen->Line(ivec2(x1, y1), ivec2(x2, y1), wm->dark_color());
      screen->Line(ivec2(x1, y1), ivec2(x1, y2), wm->dark_color());
      screen->Line(ivec2(x2, y1 + 1), ivec2(x2, y2), wm->bright_color());
      screen->Line(ivec2(x1 + 1, y2), ivec2(x2, y2), wm->bright_color());
      screen->Bar(ivec2(x1 + 1, y1 + 1), ivec2(x2 - 1, y2 - 1),
                  wm->medium_color());
    }

    if ((up && text) || (!up && !visual))
    {
        wm->font()->PutString(screen, m_pos + ivec2(4, 5), text, wm->black());
        wm->font()->PutString(screen, m_pos + ivec2(3, 4), text);
    }
    else if (up)
        screen->PutImage(visual, m_pos + ivec2(3, 3), 1);
    else
        screen->PutImage(visual, ivec2(x1 + 3, y1 + 3), 1);
}

void text_field::draw_first(image *screen)
{
  wm->font()->PutString(screen, m_pos + ivec2(0, 3), prompt);
  screen->Bar(ivec2(xstart(), m_pos.y), ivec2(xend(), yend()), wm->dark_color());
  wm->font()->PutString(screen, ivec2(xstart() + 1, m_pos.y + 3), data);
}


void text_field::draw_cur(int color, image *screen)
{
  screen->Bar(ivec2(xstart() + cur * wm->font()->Size().x + 1, yend() - 2),
              ivec2(xstart() + (cur + 1) * wm->font()->Size().x, yend() - 1),
              color);
}



info_field::info_field(int X, int Y, int ID, char const *info, ifield *Next)
{
  m_pos = ivec2(X, Y); id = ID; next = Next;
  text = strdup(info);
  w = -1;
}


void info_field::area(int &x1, int &y1, int &x2, int &y2)
{
  if (w==-1)     // if we haven't calculated this yet
  {
    int fw = wm->font()->Size().x, fh = wm->font()->Size().y, maxw = 0;
    char *info=text;
    for (w=fw,h=fh+1; *info; info++)
    {
      if (w>maxw) maxw=w;
      if (*info=='\n')
      {
    h+=fh+1;
    w=1;
      }
      else w+=fw;
    }
    w=maxw;
  }
  x1 = m_pos.x;
  y1 = m_pos.y;
  x2 = m_pos.x + w;
  y2 = m_pos.y + h;
}

void info_field::put_para(image *screen, char const *st, int dx, int dy,
              int xspace, int yspace, JCFont *font, int color)
{
  int ox=dx;
  while (*st)
  {
    if (*st=='\n')
    {
      dx=ox;
      dy+=yspace;
    }
    else
    {
      font->PutChar(screen, ivec2(dx, dy), *st, color);
      dx+=xspace;
    }
    st++;
  }
}

void info_field::draw_first(image *screen)
{
  put_para(screen, text, m_pos.x+1, m_pos.y+1, wm->font()->Size().x,
           wm->font()->Size().y, wm->font(), wm->black());
  put_para(screen, text, m_pos.x, m_pos.y, wm->font()->Size().x,
           wm->font()->Size().y, wm->font(), wm->bright_color());
}
