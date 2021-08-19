/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2014, 2021 Daniel Potter <dmpotter44@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 */

#ifndef __LISP_BINDING_H

#define __LISP_BINDING_H

#include "control_bindings.h"
#include "../lisp/lisp.h"

// Bind a lisp function to a control
class LispControl : public Control {
public:
    LispControl(LSymbol* lispFunction);
    ~LispControl();
    virtual void controlState(Sint16 state);
private:
    LSymbol* m_function;
};

void* bind_lisp_function_to_control(LString* name, LSymbol* func);
// Bind inputs - possibly a list - to a control
void* bind_inputs_to_control(LObject* obj, LString* control_name);
// Bind a single input (so if the input is a list, don't recurse into it, it's invalid)
// to a control
void* bind_input_to_control(LObject* input, LString* control_name);

LString* get_bound_controls_description(LString* control_name);

#endif