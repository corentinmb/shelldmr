#ifndef _EVALUATION_H
#define _EVALUATION_H

#include "Shell.h"

int remoteAdd(char *machine);
int remoteListe();
int remoteCmd(char *host, char * cmd);
int remoteRemove();
int redirect_a(Expression *e);
int redirect_e(Expression *e);
int redirect_eo(Expression *e);
int pipe_expr (Expression *e);
int sequence(Expression *e);
int sequence_and(Expression *e);
int sequence_or(Expression *e);
extern int evaluer_expr(Expression *e);
int exec_simple(Expression *e, int bg);
void redirection_i(Expression* e, int bg);
void redirection_o(Expression* e, int bg);
int exec_bg(Expression *e);
int exec_sous_shell(Expression *e);

#endif
