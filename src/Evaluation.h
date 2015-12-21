#ifndef _EVALUATION_H
#define _EVALUATION_H

#include "Shell.h"

extern int evaluer_expr(Expression *e);
int exec_simple(Expression *e);
int exec_bg(Expression *e);
int exec_sous_shell(Expression *e);
#endif
