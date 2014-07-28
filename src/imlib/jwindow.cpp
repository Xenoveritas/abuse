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

#include "video.h"
#include "image.h"
#include "input.h"
#include "../sdlport/event.h"
#include "filter.h"
#include "jwindow.h"
#include "sprite.h"

static int jw_left = 3, jw_right = 3, jw_top = 2, jw_bottom = 3;

int frame_top() { return jw_top; }
int frame_bottom() { return jw_bottom; }
int frame_left() { return jw_left; }
int frame_right() { return jw_right; }

//
//  Sets the size of the border around each window
//
void set_frame_size(int x)
{
    if(x < 1)
        x = 1;
    jw_left = x;
    jw_right = x;
    jw_top = 2;
    jw_bottom = x;
}

void WindowManager::grab_focus(Jwindow *j)
{
    m_grab=j;
}

void WindowManager::release_focus()
{
    m_grab = NULL;
}


void WindowManager::close_window(Jwindow *j)
{
    delete j;
}

void WindowManager::hide_windows()
{
    for (Jwindow *p = m_first; p; p = p->next)
    {
        if (!p->is_hidden())
        {
            p->hide();
            m_surf->AddDirty(p->m_pos, p->m_pos + p->m_size);
        }
    }
}

void WindowManager::show_windows()
{
  Jwindow *p;
  for (p=m_first; p; p=p->next)
    if (p->is_hidden())
      show_window(p);
}

void WindowManager::hide_window(Jwindow *j)
{
    if (j == m_first)
        m_first = m_first->next;
    else
    {
        Jwindow *k;
        for (k = m_first; k->next != j; k = k->next)
            k->m_surf->AddDirty(j->m_pos - k->m_pos,
                                j->m_pos - k->m_pos + j->m_size);
        k->m_surf->AddDirty(j->m_pos - k->m_pos,
                            j->m_pos - k->m_pos + j->m_size);
        k->next = j->next;
    }
    m_surf->AddDirty(j->m_pos, j->m_pos + j->m_size);
    j->hide();
}

void WindowManager::show_window(Jwindow *j)
{
    if (j->is_hidden())
    {
        j->show();
        j->m_surf->AddDirty(ivec2(0), j->m_size);
    }
}

void WindowManager::GetEvent(SDL_Event &ev)
{
	PollEvent(ev);

	if (state==inputing)
	{
		Jwindow *j;
		ivec2 mouse_pos = GetMousePos();
		for (m_active_window=NULL,j=m_first; j; j=j->next)
		{
			if (!j->is_hidden() && mouse_pos >= j->m_pos
				&& mouse_pos < j->m_pos + j->m_size)
			{
				m_active_window = j;
			}
		}

		if (!m_active_window && m_grab)
			m_active_window = m_grab;

		if (m_active_window)
		{
			int closew = 0, movew = 0;

			if ((ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT &&
				mouse_pos >= m_active_window->m_pos &&
				mouse_pos < m_active_window->m_pos + ivec2(m_active_window->m_size.x, m_active_window->y1())))
			{
				if (mouse_pos.x - m_active_window->m_pos.x < 11)
					closew = 1;
				else if (m_active_window->is_moveable())
					movew = 1;
			}
			else if (m_grab)
				m_active_window = m_grab;

			if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
				closew = 1;

			if (closew)
			{
				ev.type = ABUSE_EV_CLOSE_WINDOW;
			}
			else if (movew)
			{
				int red = 0;
				if (m_active_window == m_first)       // see if we need to raise the window
				{
					m_first = m_first->next;
					if (m_first)
						red = 1;
				}
				else
				{
					Jwindow *last = m_first;
					for (; last->next != m_active_window; last = last->next)
					{
					}
					if (m_active_window->next)
						red = 1;
					last->next = m_active_window->next;
				}
				if (!m_first)
					m_first = m_active_window;
				else
				{
					Jwindow *last = m_first;
					for (; last->next; last = last->next);
					last->next = m_active_window;
				}
				m_active_window->next = NULL;
				if (red)
				{
					Jwindow *j = m_active_window;
					j->m_surf->AddDirty(ivec2(0), j->m_size);
					FlushScreen();
				}
				state = dragging;
				drag_window = m_active_window;
				drag_mousex = m_active_window->m_pos.x - mouse_pos.x;
				drag_mousey = m_active_window->m_pos.y - mouse_pos.y;
				ev.type = ABUSE_EV_SPURIOUS;
			}
			else if (m_active_window)
			{
				m_active_window->inm->handle_event(ev, m_active_window);
			}
		}
	}
	else if (state==dragging)
	{
		m_active_window=drag_window;
		if (ev.type==SDL_MOUSEBUTTONUP)  // user released the mouse
		{
			state=inputing;
			ev.type = ABUSE_EV_SPURIOUS;
		}
		else if (ev.type==SDL_MOUSEMOTION)
		{
			move_window(drag_window, ev.motion.x + drag_mousex, ev.motion.y + drag_mousey);
			FlushScreen();
			// Drag window was never processed, so just make it "spurious"
			ev.type = ABUSE_EV_SPURIOUS;
		}
	}
}

void Jwindow::Resize(ivec2 size)
{
    m_surf->SetSize(size);
    m_size = size;
}

void WindowManager::resize_window(Jwindow *j, int l, int h)
{
  Jwindow *p;
  m_surf->AddDirty(j->m_pos, j->m_pos + j->m_size);
  for (p=m_first; p!=j; p=p->next)
    p->m_surf->AddDirty(j->m_pos - p->m_pos,
                        j->m_pos - p->m_pos + j->m_size);
  j->Resize(ivec2(l,h));
  if (!frame_suppress)
  j->redraw();
}

void WindowManager::move_window(Jwindow *j, int x, int y)
{
    m_surf->AddDirty(j->m_pos, j->m_pos + j->m_size);
    for(Jwindow *p = m_first; p != j; p = p->next)
        p->m_surf->AddDirty(j->m_pos - p->m_pos,
                            j->m_pos - p->m_pos + j->m_size);
    j->m_pos.x = x;
    j->m_pos.y = y;
    j->m_surf->AddDirty(ivec2(0), j->m_size);
}

WindowManager::WindowManager(image *screen, palette *pal, int Hi,
                             int Med, int Low, JCFont *Font)
  : EventHandler(screen, pal)
{
    wm = this;
    m_surf = screen;
    hi = Hi; low = Low; med = Med; m_first = NULL; m_pal = pal; m_grab = NULL;
    bk = pal->find_closest(0, 0, 0);
    state = inputing; fnt = Font;  wframe_fnt = Font;
    frame_suppress = 0;
}

WindowManager::~WindowManager()
{
    while(m_first)
        close_window(m_first);
    wm = NULL;
}

void WindowManager::add_window(Jwindow *win)
{
    if(!m_first)
        m_first = win;
    else
    {
        Jwindow *tmp = m_first;
        while(tmp->next)
            tmp = tmp->next;
        tmp->next = win;
        win->next = NULL;
    }
}

void WindowManager::remove_window(Jwindow *win)
{
    if(m_grab == win)
        m_grab = NULL;

    // close the window we were dragging
    if(state == dragging && win == drag_window)
        state = inputing;

    if(m_first == win)
        m_first = m_first->next;
    else
    {
        Jwindow * search;
        for(search = m_first; search->next != win; search = search->next)
            search->m_surf->AddDirty(win->m_pos - search->m_pos,
                                     win->m_pos - search->m_pos + win->m_size);
        search->m_surf->AddDirty(win->m_pos - search->m_pos,
                                 win->m_pos - search->m_pos + win->m_size);
        search->next = win->next;
    }

    m_surf->AddDirty(win->m_pos, win->m_pos + win->m_size);
}

Jwindow * WindowManager::CreateWindow(ivec2 pos, ivec2 size,
                                      ifield * fields, char const *name)
{
    if(pos.x > m_surf->Size().x - 4)
        pos.x = m_surf->Size().x - 25;
    if(pos.y > m_surf->Size().y - 4)
        pos.y = m_surf->Size().y - 10;

    Jwindow * j = new Jwindow(pos, size, fields, name);
    j->show();

    return j;
}

int WindowManager::KeyPressed(SDL_Keycode keycode)
{
    int numkeys;
    const Uint8* keymap = SDL_GetKeyboardState(&numkeys);
    SDL_Scancode scancode = SDL_GetScancodeFromKey(keycode);
    if (scancode >= numkeys)
        return 0;
    else
        return keymap[scancode];
}

void WindowManager::flush_screen()
{
    ivec2 m1(0, 0);

    m1 = m_pos - m_center;
    ivec2 m2 = m1 + m_sprite->m_visual->Size();

    m_sprite->m_save->PutPart(m_surf, ivec2(0, 0), m1, m2);
    m_surf->PutImage(m_sprite->m_visual, m1, 1);

    for (Jwindow *p = m_first; p; p = p->next)
        if (!p->is_hidden())
            m_surf->DeleteDirty(p->m_pos, p->m_pos + p->m_size);
    update_dirty(m_surf);

    m_surf->PutImage(m_sprite->m_save, m1);

    for (Jwindow *p = m_first; p; p = p->next)
    {
        if (p->is_hidden())
            continue;

        m_sprite->m_save->PutPart(p->m_surf, ivec2(0, 0), m1 - p->m_pos,
                                  m1 - p->m_pos + m_sprite->m_visual->Size());
        p->m_surf->PutImage(m_sprite->m_visual, m1 - p->m_pos, 1);

//      m_surf->DeleteDirty(p->m_pos, p->m_pos + p->m_size);
        for (Jwindow *q = p->next; q; q = q->next)
            if (!q->is_hidden())
                p->m_surf->DeleteDirty(q->m_pos - p->m_pos,
                                       q->m_pos - p->m_pos + q->m_size);
        update_dirty(p->m_surf, p->m_pos.x, p->m_pos.y);
        p->m_surf->PutImage(m_sprite->m_save, m1 - p->m_pos, 0);
    }
}

Jwindow::Jwindow(char const *name)
{
    _x1 = left_border();
    _y1 = jw_top + 5;
    _x2 = _y2 = 0;

    _hidden = true;
    _moveable = true;
    // property.flags = JWINDOW_NOAUTOHIDE_FLAG;

    inm = new InputManager(this, NULL);
    reconfigure();

    m_surf = NULL;
    next = NULL;

    _name = NULL;
    if(name)
        _name = strdup(name);
    wm->add_window(this);
}

Jwindow::Jwindow(ivec2 pos, ivec2 size, ifield *f, char const *name)
{
    m_size = 0;
    _hidden = false;
    _moveable = true;

    _x1 = left_border();
    _y1 = name ? top_border() : jw_top + 5;

    m_surf = NULL;
    inm = new InputManager(m_surf, f);
    reconfigure(); /* FIXME: TODO */

    m_size.x = size.x >= 0 ? size.x + left_border() : m_size.x - size.x;
    m_size.y = size.y >= 0 ? size.y + top_border() : m_size.y - size.y;
    m_pos.x = pos.x >= 0 ? pos.x : xres - m_size.x + pos.x - left_border() - right_border() - 1;
    m_pos.y = pos.y >= 0 ? pos.y : yres - m_size.y + pos.y - top_border() - bottom_border() - 1;

    backg = wm->medium_color();

    _x2 = m_size.x - 1;
    _y2 = m_size.y - 1;
    m_size += ivec2(right_border(), bottom_border());

    if(size.x == -1)
        m_size.x = Max(m_size.x, 15 + left_border() + right_border());
    if(size.y == -1)
        m_size.y = Max(m_size.y, top_border() + bottom_border());
    m_surf = new image(m_size, NULL, 2);
    m_surf->clear(backg);
    // Keep this from getting destroyed when image list is cleared
    image_list.unlink(m_surf);
    inm->m_surf = m_surf;

    next = NULL;

    _name = NULL;
    if(name)
        _name = strdup(name);

    wm->add_window(this);
    if(!wm->frame_suppress)
        redraw();
}

Jwindow::~Jwindow()
{
    wm->remove_window(this);
    local_close();
    delete m_surf;
    delete inm;
    if(_name)
        free(_name);
}

void Jwindow::reconfigure()
{
    int x1, y1, x2, y2;
    ifield *i;
    m_size = ivec2(2);
    for(i = inm->m_first; i; i = i->next)
    {
        i->set_owner(this);
        i->area(x1, y1, x2, y2);
        m_size = Max(m_size, ivec2(x2, y2));
    }
}

void Jwindow::local_close()
{
    ;
}

void Jwindow::redraw()
{
    int hi = wm->bright_color();
    int med = wm->medium_color();
    int low = wm->dark_color();
    JCFont * fnt = wm->frame_font();

    if(_name)
    {
        if (right_border() >= 1)
        {
            m_surf->WidgetBar(ivec2(0, 0), m_size - ivec2(1), hi, med, low);
            if (right_border() >= 3)
                m_surf->WidgetBar(ivec2(right_border() - 1, top_border() - 1),
                                  m_size - ivec2(left_border(), bottom_border()),
                                  low, med, hi);

          else
            m_surf->Line(ivec2(left_border() - 1, top_border() - 1),
                         ivec2(right_border() - 1, top_border() - 1), low);
        }
      m_surf->Rectangle(ivec2(2, 2), ivec2(top_border() - 2, top_border() - 3),
                        wm->black());
      m_surf->WidgetBar(ivec2(3, 3), ivec2(top_border() - 3, top_border() - 4),
                        hi, med, low); // draws the 'close' button
    }

  else
    {
      if (right_border() >= 1)
        {
          m_surf->WidgetBar(ivec2(0, 0), m_size - ivec2(1), hi, med, low);
          if (right_border() >= 3)
            m_surf->WidgetBar(ivec2(right_border() - 1, jw_top + 4),
                              m_size - ivec2(left_border(), bottom_border()),
                              low, med, hi);
          else
            m_surf->Line(ivec2(left_border() - 1, jw_top + 4),
                         ivec2(right_border() - 1, jw_top + 4), low);
        }
      // Draw the 'close' button
      m_surf->Rectangle(ivec2(1, 1), ivec2(4, 4), wm->black ());
      m_surf->WidgetBar(ivec2(2, 2), ivec2(3, 3), hi, med, low);
    }
  if (_name && _name[0])
    {
      m_surf->Bar(ivec2(top_border(), 1),
                  ivec2(top_border() + fnt->Size().x * strlen (_name) + 1,
                        top_border() - 2),
                  med);
      fnt->PutString(m_surf, ivec2(top_border() + 1, 1), _name, low);
    }
  // clear 'client' region
  m_surf->Bar(ivec2(x1(), y1()), ivec2(x2(), y2()), backg);
  inm->redraw ();
}

int Jwindow::left_border()
{
    return frame_left();
}

int Jwindow::right_border()
{
    return frame_right();
}

int Jwindow::top_border()
{
    return wm->font()->Size().y + frame_top();
}

int Jwindow::bottom_border()
{
    return frame_bottom();
}

ifield *InputManager::unlink(int id)     // unlinks ID from fields list and return the pointer to it
{
  for (ifield *i=m_first,*last=NULL; i; i=i->next)
  {
    if (i->id==id)
    {
      if (i==m_first)
    m_first=m_first->next;
      else
        last->next=i->next;
      if (m_active==i)
        m_active=m_first;
      return i;
    }
    ifield *x=i->unlink(id);
    if (x) return x;
    last=i;
  }
  return NULL;   // no such id
}

InputManager::~InputManager()
{ ifield *i;
  while (m_first)
  { i=m_first;
    m_first=m_first->next;
    delete i;
  }
}

void InputManager::clear_current()
{
    if(m_owner)
        m_surf = m_owner->m_surf;
    if(m_active)
        m_active->draw(0, m_surf);
    m_active = NULL;
}

void InputManager::handle_event(SDL_Event &ev, Jwindow *j)
{
	ifield *i,*in_area=NULL;
	int x1,y1,x2,y2;
	ivec2 mouse_pos = wm->GetMousePos();

	if(m_owner)
		m_surf = m_owner->m_surf;

	if (j)
	{
		mouse_pos -= j->m_pos;
		m_cur = j;
	}

	if (!m_grab)
	{
		if ((ev.type==SDL_MOUSEBUTTONDOWN && ev.button.button==SDL_BUTTON_LEFT) || ev.type==SDL_MOUSEMOTION)
		{
			for (i=m_first; i; i=i->next)
			{
				i->area(x1,y1,x2,y2);
				if (mouse_pos.x>=x1 && mouse_pos.y>=y1 &&
					mouse_pos.x<=x2 && mouse_pos.y<=y2)
				{
					in_area=i;
				}
			}
			if (in_area!=m_active && (no_selections_allowed || (in_area && in_area->selectable())))
			{
				if (m_active)
					m_active->draw(0,m_surf);
				m_active=in_area;

				if (m_active)
					m_active->draw(1,m_surf);
			}
		}
		if (ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_TAB && m_active)
		{
			m_active->draw(0,m_surf);
			do
			{
				m_active=m_active->next;
				if (!m_active)
					m_active=m_first;
			}
			while (m_active && !m_active->selectable());
			m_active->draw(1,m_surf);
		}
	}
	else
	{
		m_active=m_grab;
	}

	if (m_active)
	{
		if (ev.type != SDL_MOUSEMOTION && ev.type != SDL_MOUSEBUTTONDOWN && ev.type != SDL_MOUSEBUTTONUP)
			m_active->handle_event(ev,m_surf,this);
		else
		{
			m_active->area(x1,y1,x2,y2);
			if (m_grab || (mouse_pos.x>=x1 && mouse_pos.y>=y1 &&
				mouse_pos.x<=x2 && mouse_pos.y<=y2))
			{
				if (j)
					m_active->handle_event(ev,m_surf,j->inm);
				else
					m_active->handle_event(ev,m_surf,this);
			}
		}
	}
}

void InputManager::allow_no_selections()
{
    no_selections_allowed=1;
}

void InputManager::redraw()
{
    ifield *i;
    if(m_owner)
        m_surf = m_owner->m_surf;
    for(i = m_first; i; i = i->next)
        i->draw_first(m_surf);
    if(m_active)
        m_active->draw(1, m_surf);
}

InputManager::InputManager(image *screen, ifield *first)
{
    no_selections_allowed = 0;
    m_cur = NULL;
    m_grab = NULL;
    m_owner = NULL;
    m_surf = screen;
    m_active = m_first = first;
    while(m_active && !m_active->selectable())
        m_active = m_active->next;
    if(m_surf)
        redraw();
}

InputManager::InputManager(Jwindow *owner, ifield *first)
{
    no_selections_allowed = 0;
    m_cur = NULL;
    m_grab = NULL;
    m_owner = owner;
    m_surf = NULL;
    m_active = m_first = first;
    while(m_active && !m_active->selectable())
        m_active = m_active->next;
}

void InputManager::grab_focus(ifield *i)
{
    m_grab = i;
    if (m_cur)
		wm->grab_focus(m_cur);
}

void InputManager::release_focus()
{
    m_grab = NULL;
    if (m_cur)
        wm->release_focus();
}

void InputManager::remap(Filter *f)
{
    for (ifield *i = m_first; i; i = i->next)
        i->remap(f);
    redraw();
}

void InputManager::add(ifield *i)
{ ifield *f=m_first;
  if (i->selectable())
  {
    if (!f)
      m_first=i;
    else
    {
      while (f->next) f=f->next;
      f->next=i;
    }
  }
}

ifield *InputManager::get(int id)
{
  ifield *f;
  for (f=m_first; f; f=f->next)
  {
    ifield *ret=f->find(id);
    if (ret) return ret;
  }
  return NULL;
}

ifield::ifield()
{
    owner = NULL;
    m_pos = ivec2(0, 0);
    next = NULL;
    id = 0;
}

ifield::~ifield()
{
    ;
}

/* re-position the control with respect to the "client" area of the window */
void ifield::set_owner(Jwindow * newowner)
{
    if(owner)
        Move(m_pos - ivec2(owner->x1(), owner->y1()));
    owner = newowner;
    if(owner)
        Move(m_pos + ivec2(owner->x1(), owner->y1()));
}
