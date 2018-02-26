/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __LISP_HPP_
#define __LISP_HPP_

#include <cstdlib>
#include <stdint.h>

#ifdef L_PROFILE
#include "timing.h"
#endif

#define Cell void
#define MAX_LISP_TOKEN_LEN 200

#define FIXED_TRIG_SIZE 360               // 360 degrees stored in table
extern int32_t sin_table[FIXED_TRIG_SIZE];   // this should be filled in by external module
#define TBS 1662                          // atan table granularity
extern uint16_t atan_table[TBS];
#define NILP(x) ((x)==NULL)
#define DEFINEDP(x) ((x)!=l_undefined)
class bFILE;
extern bFILE *current_print_file;

enum
{
    L_BAD_CELL, // error catching type
    L_CONS_CELL,
    L_NUMBER,
    L_SYMBOL,
    L_SYS_FUNCTION,
    L_USER_FUNCTION,
    L_STRING,
    L_CHARACTER,
    L_C_FUNCTION,
    L_C_BOOL,
    L_L_FUNCTION,
    L_POINTER,
    L_OBJECT_VAR,
    L_1D_ARRAY,
    L_FIXED_POINT,
    L_COLLECTED_OBJECT,
};

// FIXME: switch this to uint8_t one day? it still breaks stuff
typedef uint8_t ltype;

struct LSpace
{
    size_t GetFree();
    void *Alloc(size_t size);

    void *Mark();
    void Restore(void *val);
    void Clear();

    static LSpace Tmp, Perm, Gc;
    static LSpace *Current;

    uint8_t *m_data;
    uint8_t *m_free;
    char const *m_name;
    size_t m_size;
};

struct LObject
{
    /* Factories */
    static LObject *Compile(char const *&s);

    /* Methods */
    LObject *Eval();
    void Print();

    /* Members */
    ltype m_type;
};

struct LObjectVar : LObject
{
    /* Factories */
    static LObjectVar *Create(int index);

    /* Members */
    int m_index;
};

struct LList : LObject
{
    /* Factories */
    static LList *Create();

    /* Methods */
    size_t GetLength();
    LList *Assoc(LObject *item);

    /* Members */
    LObject *m_cdr, *m_car;
};

struct LNumber : LObject
{
    /* Factories */
    static LNumber *Create(long num);

    /* Members */
    long m_num;
};

struct LRedirect : LObject
{
    /* Members */
    LObject *m_ref;
};

struct LString : LObject
{
    /* Factories */
    static LString *Create(char const *string);
    static LString *Create(char const *string, int length);
    static LString *Create(int length);

    /* Methods */
    char *GetString();

    /* Members */
private:
    char m_str[1]; /* Can be allocated much larger than 1 */
};

struct LSymbol : LObject
{
    /* Factories */
    static LSymbol *Find(char const *name);
    static LSymbol *FindOrCreate(char const *name);

    /* Methods */
    LObject *EvalFunction(void *arg_list);
    LObject *EvalUserFunction(LList *arg_list);

    LString *GetName();
    LObject *GetFunction();
    LObject *GetValue();

    void SetFunction(LObject *fun);
    void SetValue(LObject *value);
    void SetNumber(long num);

    /* Members */
#ifdef L_PROFILE
    float time_taken;
#endif
    LObject *m_value;
    LObject *m_function;
    LString *m_name;
    LSymbol *m_left, *m_right; // tree structure

    /* Static members */
    static LSymbol *root;
    static size_t count;
};

struct LSysFunction : LObject
{
    /* Methods */
    LObject *EvalFunction(LList *arg_list);

    /* Members */
    short min_args, max_args;
    short fun_number;
};

struct LUserFunction : LObject
{
    LList *arg_list, *block_list;
};

struct LArray : LObject
{
    /* Factories */
    static LArray *Create(size_t len, void *rest);

    /* Methods */
    inline LObject **GetData() { return m_data; }
    LObject *Get(int x);

    /* Members */
    size_t m_len;

private:
    LObject *m_data[1]; /* Can be allocated much larger than 1 */
};

struct LChar : LObject
{
    /* Factories */
    static LChar *Create(uint16_t ch);

    /* Methods */
    uint16_t GetValue();

    /* Members */
    uint16_t m_ch;
};

struct LPointer : LObject
{
    /* Factories */
    static LPointer *Create(void *addr);

    /* Members */
    void *m_addr;
};

struct LFixedPoint : LObject
{
    /* Factories */
    static LFixedPoint *Create(int32_t x);

    /* Members */
    int32_t m_fixed;
};

class Lisp
{
public:
    static void Init();
    static void Uninit();

    static void InitConstants();

    // Collect temporary or permanent spaces
    static void CollectSpace(LSpace *which_space, int grow);

private:
    static LArray *CollectArray(LArray *x);
    static LList *CollectList(LList *x);
    static LObject *CollectObject(LObject *x);
    static void CollectSymbols(LSymbol *root);
    static void CollectStacks();
};

static inline LObject *&CAR(void *x) { return ((LList *)x)->m_car; }
static inline LObject *&CDR(void *x) { return ((LList *)x)->m_cdr; }

#ifdef __GNUC__
/*
 * C++ spec says "this" is always NON-NULL, recent versions of gcc will warn
 * about this and optimizes the "if (this)" we use in some places away:
 * "warning: nonnull argument ‘this’ compared to NULL [-Wnonnull-compare]"
 * We rely on "if (this)" checks in several places and refactoring this is
 * non trivial. So we use this little helper marked with
 * __attribute__((optimize("O0"))) to workaround this.
 */
static inline bool __attribute__((optimize("O0"))) ptr_is_null(void *ptr)
{
    return ptr == NULL;
}
#else
static inline bool ptr_is_null(void *ptr)
{
    return ptr == NULL;
}
#endif

static inline ltype item_type(void *x) { if (!ptr_is_null(x)) return *(ltype *)x; return L_CONS_CELL; }

void perm_space();
void tmp_space();
void *lpointer_value(void *lpointer);
int32_t lnumber_value(void *lnumber);
long lfixed_point_value(void *c);
void *lisp_atom(void *i);
LObject *lcdr(void *c);
LObject *lcar(void *c);
void *lisp_eq(void *n1, void *n2);
void *lisp_equal(void *n1, void *n2);
void *eval_block(void *list);
void resize_tmp(size_t new_size);
void resize_perm(size_t new_size);

void push_onto_list(void *object, void *&list);
LSymbol *add_c_object(void *symbol, int index);
LSymbol *add_c_function(char const *name, short min_args, short max_args, short number);
LSymbol *add_c_bool_fun(char const *name, short min_args, short max_args, short number);
LSymbol *add_lisp_function(char const *name, short min_args, short max_args, short number);
int read_ltoken(char *&s, char *buffer);
void print_trace_stack(int max_levels);


LSysFunction *new_lisp_sys_function(int min_args, int max_args, int fun_number);
LSysFunction *new_lisp_c_function(int min_args, int max_args, int fun_number);
LSysFunction *new_lisp_c_bool(int min_args, int max_args, int fun_number);

LUserFunction *new_lisp_user_function(LList *arg_list, LList *block_list);

LSysFunction *new_user_lisp_function(int min_args, int max_args, int fun_number);

int end_of_program(char *s);

void *nth(int num, void *list);
int32_t lisp_atan2(int32_t dy, int32_t dx);
int32_t lisp_sin(int32_t x);
int32_t lisp_cos(int32_t x);

extern "C" {
void lbreak(const char *format, ...);
} ;

extern void clisp_init();                      // external initalizer call by lisp_init()
extern long c_caller(long number, void *arg);  // exten c function switches on number
extern void *l_caller(long number, void *arg);  // exten lisp function switches on number

extern void *l_obj_get(long number);  // exten lisp function switches on number
extern void l_obj_set(long number, void *arg);  // exten lisp function switches on number
extern void l_obj_print(long number);  // exten lisp function switches on number

// FIXME: get rid of this later
static inline LObject *symbol_value(void *sym) { return ((LSymbol *)sym)->GetValue(); }
static inline char *lstring_value(void *str) { return ((LString *)str)->GetString(); }

#include "lisp_opt.h"

#endif
