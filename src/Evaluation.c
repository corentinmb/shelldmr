#include "Shell.h"
#include "Evaluation.h"
#include "Commandes_Internes.h"

/* Savoir si une commande est interne ou externe ?
typedef enum type {
  INTERNE,
  EXTERNE
} type;

type
type_commande(char *commande){

  char *out,*com;
  com="type ";
  strcat(com,commande);
  FILE *fp = popen(com, "r");

  fscanf(fp, "%s", out);
  pclose(fp);
  fprintf("%s",out);
  return INTERNE;
}
*/
int
evaluer_expr(Expression *e)
{
  if (e == NULL) return 1;

  switch(e->type){

    case VIDE :
    case SIMPLE :
    return exec_simple(e);
    case REDIRECTION_I:         // Redirection entree
    case REDIRECTION_O:         // Redirection sortie standard
    case REDIRECTION_A:         // Redirection sortie standard, mode append
    case REDIRECTION_E:         // Redirection sortie erreur
    case REDIRECTION_EO:        // Redirection sortie standard + erreur
    case BG:
    return exec_bg(e);
    case SOUS_SHELL:
    return exec_sous_shell(e);
    default :
    return 1;
  }

  return 0;
}

int
exec_simple(Expression *e){
  return 0;
}

int
exec_bg(Expression *e){
  return 0;
}

int
exec_sous_shell(Expression *e){
  return 0;
}
