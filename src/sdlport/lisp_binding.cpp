/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2021 Daniel Potter <dmpotter44@gmail.com>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "lisp_binding.h"
#include "../lisp/lisp_gc.h"
// For printing lisp errors
#include "../imlib/dprint.h"

extern Bindings control_bindings;

LispControl::LispControl(LSymbol* lispFunction) : Control(), m_function(lispFunction) {}
LispControl::~LispControl() {
    m_function = NULL;
}

void LispControl::controlState(Sint16 state) {
    // Basically just fire this on down to the Lisp interpreter
    // State is very nearly a Lisp fixed point anyway - fixed point values in Abuse
    // are 16 bits for integer part and 16 bits for fractional part.
    int32_t lispValue = state << 1;
    // Basically, if the control value is greater than 0.5, bump it up by 2, so
    // what was 0x7FFF becomes 0xFFFE (via left shift 1) and then 0x10000, or
    // 1.0
    if (lispValue > 0x7FFF) {
        lispValue += 2;
    }
    // And now to invoke the bound function
    // Mark start of temp space to later reclaim created objects
    void *m = LSpace::Tmp.Mark();
    LList *lispState;

    lispState = LList::Create();
    PtrRef r1(lispState);
    lispState->m_car = LFixedPoint::Create(lispValue);

    m_function->EvalUserFunction(lispState);

    LSpace::Tmp.Restore(m);
}

void* bind_lisp_function_to_control(LString* name, LSymbol* func) {
    std::string nameStr(name->GetString());
    LispControl* control = new LispControl(func);
    control_bindings.addControl(nameStr, *control);
    return true_symbol;
}

void* bind_inputs_to_control(LObject* obj, LString* control_name) {
    switch (obj->m_type) {
    case L_STRING:
    case L_NUMBER:
    case L_FIXED_POINT:
        return bind_input_to_control(obj, control_name);
    case L_CONS_CELL:
        {
            // Eval each item in the list and then bind it as a regular input
            long count = 0;
            for (LList* list = (LList*) obj; list != NULL && list->m_type == L_CONS_CELL; list = (LList*) list->m_cdr) {
                if (bind_input_to_control(list->m_car->Eval(), control_name) != NULL) {
                    count++;
                }
            }
            return count == 0 ? NULL : LNumber::Create(count);
        }
    default:
        obj->Print();
        lbreak(" is not a valid input");
        return NULL;
    }
    return true_symbol;
}

void* bind_input_to_control(LObject* input, LString* control_name) {
    std::string nameStr(control_name->GetString());
    switch (input->m_type) {
    case L_STRING:
        // String: parse binding using Input
        {
            const char* input_str = ((LString*)input)->GetString();
            Input input(input_str);
            if (!input.valid()) {
                dprintf("Unable to parse \"%s\" as a control input (binding ignored).\n", input_str);
                return NULL;
            }
            return control_bindings.bind(input, nameStr) ? true_symbol : NULL;
        }
    case L_NUMBER:
    case L_FIXED_POINT:
        // Number: a scancode (lnumber_value converts fixed point to integer)
        return control_bindings.bindScancode((SDL_Scancode)lnumber_value(input), nameStr) ? true_symbol : NULL;
    default:
        input->Print();
        lbreak(" is not a valid input");
        return NULL;
    }
}

LString* get_bound_controls_description(LString* control_name) {
    std::string description("Unbound");
    control_bindings.createBindingDescription(control_name->GetString(), description);
    return LString::Create(description.c_str());
}