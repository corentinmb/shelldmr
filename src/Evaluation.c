#include "Shell.h"
#include "Evaluation.h"
#include "Commandes_Internes.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <readline/history.h>

int indice = 0;

/*
 * Structure d'un shell distant
 */
struct distant_shell
{
	int tube_in[2];
	int tube_out[2];
	char hostname[40];
	int pid_local_shell;
};

struct distant_shell tab_shell[20];

/*
 * Fonction d'ajout d'un shell distant
 */
int remoteAdd(char *machine)
{
	pipe(tab_shell[indice].tube_in);
	pipe(tab_shell[indice].tube_out);

	if(!fork())
	{
		dup2(tab_shell[indice].tube_in[0], 0);
		dup2(tab_shell[indice].tube_out[1], 1);

		char buf[256];
		getcwd(buf, sizeof(buf));
		sprintf(buf, "%s/./Shell", buf);
		execlp("ssh", "ssh", machine, buf, NULL);
		return -1;
	}

	if(!(tab_shell[indice].pid_local_shell = fork()))
	{
		dup2(tab_shell[indice].tube_out[0], 0);
		execlp("./xcat.sh", "./xcat.sh", NULL);
		return -2;
	}

	strcpy(tab_shell[indice++].hostname, machine);
	return 0;
}

/*
 * Fonction de listage des shells distants
 */
int remoteListe()
{
	if(indice != 0)
	{
		int i;
		for(i = 0; i < indice; i++)
			printf("%s\n", tab_shell[i].hostname);

		return 0;
	}
	else
		return -1;
}

/*
 * Fonction permettant l'envoi de commande vers un shell distant
 */
int remoteCmd(char *host, char * cmd)
{

	int i, found = 0;
	for(i = 0; i < indice; i++)
	{
		if(strcmp(tab_shell[i].hostname, host) == 0)
		{
			sprintf(cmd, "%s\n", cmd);
			write(tab_shell[i].tube_in[1], cmd, sizeof(cmd));
			write(tab_shell[i].tube_out[1], cmd, sizeof(cmd));
			found =  1;
		}
	}

	if(!found)
		printf("Erreur machine invalide\n");

	return 0;
}

/*
 * Fonction de suppression des shells distants
 */
int remoteRemove()
{

	int i;
	for(i = 0; i < indice; i++)
	{
		write(tab_shell[i].tube_in[1], "exit\n", sizeof("exit\n"));
		close(tab_shell[i].tube_in[1]);
		close(tab_shell[i].tube_in[0]);
		close(tab_shell[i].tube_out[1]);
		close(tab_shell[i].tube_out[0]);
	}
	system("killall xterm"); // Pas la meilleure solution...
	indice = 0;
	return 0;
}

/*
 * Fonction d'exécution d'une commande simple
 */
int executer_simple(Expression *e)
{
	if(strcmp(e->arguments[0], "kill") == 0){
		if(kill2(LongueurListe(e->arguments),e->arguments))
			printf("Erreur dans la commande kill\n");
		return 0;
	}
	else if(strcmp(e->arguments[0], "exit") == 0)
		exit2();
	else if(strcmp(e->arguments[0], "pwd") == 0)
		return pwd2();
	else if(strcmp(e->arguments[0], "hostname") == 0)
		return hostname2();
	else if(strcmp(e->arguments[0], "echo") == 0)
		return echo2(e->arguments[1]);
	else if(strcmp(e->arguments[0], "date") == 0)
		return date2();
	else if(strcmp(e->arguments[0], "history") == 0)
		return history2();
	else if(strcmp(e->arguments[0], "cd") == 0)
		return cd2(e->arguments[1]);
	else if(strcmp(e->arguments[0], "remote") == 0)
	{
		if(strcmp(e->arguments[1], "list") == 0)
		{
			if(remoteListe())
				printf("Erreur, aucune machine connecté en ssh\n");
			return 1;
		}
		else if(strcmp(e->arguments[1], "add") == 0)
		{
			for(int i=2; i < LongueurListe(e->arguments);i++){
				if(remoteAdd(e->arguments[i]))
					printf("Erreur, impossible de se connecter\n");
			}
			return 2;
		}
		else if(strcmp(e->arguments[1], "remove") == 0)
			return remoteRemove();
		else if(strcmp(e->arguments[1], "all") == 0)
		{
			if(indice != 0)
			{
				int i;
				for(i = 0; i < indice; i++){
					if(remoteCmd(tab_shell[i].hostname, e->arguments[2]))
						printf("Erreur, hote ou commande invalide\n");
				}
			}
			return 3;
		}
		else
		{
			if(remoteCmd(e->arguments[1], e->arguments[2]))
				printf("Erreur, hote ou commande invalide\n");
			return 4;
		}
	}
	else
	{
		int pid = fork();
        prctl(PR_SET_PDEATHSIG, SIGHUP);
		if(pid == 0){
			execvp(e->arguments[0], e->arguments);
		    exit(1);
		}
		else if(pid > 0)
		{
		    int status;
				waitpid(pid, &status, 0);
				return (WIFEXITED(status)? WEXITSTATUS(status) : -1);
		}
		else
			printf("Erreur\n");
			return -1;
	}
}

/*
 * Fonction d'exécution d'un sous-shell
 */
int exec_sous_shell(Expression *e){
    return evaluer_expr(e->gauche);
}


/*
 * Fonction de mise en arrière-plan
 */
int exec_bg(Expression *e){
    int pid;
    if ((pid = fork()) == 0){
        exit(evaluer_expr(e->gauche));
    }
    else if (pid > 0 ){
        return 0;
    }
    else {
        return -1;
    }
}

/*
 * Fonction de redirection en entrée <
 */
int redirect_i(Expression *e)
{
    int pid,status;
    if ((pid = fork()) == 0)
    {
        int fd = open(e->arguments[0], O_RDONLY, 0666);
        dup2(fd, STDIN_FILENO);
        close(fd);
        status = evaluer_expr(e->gauche);
        exit(status);
    }
    else
    {
        waitpid(pid, &status, 0);
        return (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    }
}

/*
 * Fonction de redirection en sortie >
 */
int redirect_o(Expression *e)
{
    int pid,status;
    if ((pid = fork()) == 0)
    {
        int fd = open(e->arguments[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        dup2(fd, STDOUT_FILENO);
        close(fd);
        status = evaluer_expr(e->gauche);
        exit(status);
    }
    else
    {
        waitpid(pid, &status, 0);
        return (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    }
}

/*
 * Fonction de redirection en sortie en mode APPEND >>
 */
int redirect_a(Expression *e)
{
    int pid,status;
    if ((pid = fork()) == 0)
    {
        int fd = open(e->arguments[0], O_WRONLY | O_CREAT | O_APPEND, 0666);
        dup2(fd, STDOUT_FILENO);
        close(fd);
        status = evaluer_expr(e->gauche);
        exit(status);
    }
    else
    {
        waitpid(pid, &status, 0);
        return (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    }
}

/*
 * Fonction de redirection sur la sortie d'erreur
 */
int redirect_e(Expression *e)
{
    int pid,status;
    if ((pid = fork()) == 0)
    {
        int fd = open(e->arguments[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        dup2(fd, STDERR_FILENO);
        close(fd);
        status = evaluer_expr(e->gauche);
        exit(status);
    }
    else
    {
        waitpid(pid, &status, 0);
        return (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    }
}

/*
 * Fonction de redirection sur les sorties standard et d'erreur
 */
int redirect_eo(Expression *e)
{
    int pid,status;
    if ((pid = fork()) == 0)
    {
        int fd = open(e->arguments[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        dup2(fd, STDERR_FILENO);
        dup2(fd, STDOUT_FILENO);
        close(fd);
        status = evaluer_expr(e->gauche);
        exit(status);
    }
    else
    {
        waitpid(pid, &status, 0);
        return (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    }
}

/*
 * Fonction d'exécution des commandes séparées par un pipe
 */
int pipe_expr (Expression *e)
{
    int pid,status;
    if ((pid = fork()) == 0)
    {
        int tube[2];
        int pid_1, pid_2;
        pipe(tube);
        if ((pid_1 = fork()) == 0)
        {
            dup2(tube[1], STDOUT_FILENO);
            close(tube[1]);
            status = evaluer_expr(e->gauche);
            exit(status);
        }
        else
        {
            close(tube[1]);
            if ((pid_2 = fork()) == 0)
            {
                dup2(tube[0], STDIN_FILENO);
                close(tube[0]);
                status = evaluer_expr(e->droite);
                exit(status);
            }
            else
            {
                close(tube[0]);
                waitpid(pid_1, &status, 0);
                waitpid(pid_2, &status, 0);
                exit(0);
            }
        }
    }
    else
    {
        waitpid(pid, &status, 0);
        return (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    }

}

/*
 * Fonction d'exécution des commandes séparées par un point virgule
 */
int sequence(Expression *e)
{
    evaluer_expr(e->gauche);
    return evaluer_expr(e->droite);
}

/*
 * Fonction d'exécution des commandes séparées par une double esperluette
 */
int sequence_and(Expression *e)
{
    int status = evaluer_expr(e->gauche);
    if ( status == 0)
        return evaluer_expr(e->droite);
    else
        return status;
}

/*
 * Fonction d'exécution des commandes séparées par un double pipe
 */
int sequence_or(Expression *e)
{
    int status = evaluer_expr(e->gauche);
    if ( status == 0)
        return 0;
    else
        return evaluer_expr(e->droite);

}

/*
 * Fonction d'évaluation de l'expression
 */
int evaluer_expr(Expression *e)
{

  if (e == NULL) return 0;

  switch(e->type)
  {

  case VIDE :
    break ;

  case SIMPLE :
	return executer_simple(e);
	break;
 case BG:
  	return exec_bg(e);
    break;

  case SOUS_SHELL : return exec_sous_shell(e);
  case REDIRECTION_I: return redirect_i(e);
  case REDIRECTION_O: return redirect_o(e);
  case REDIRECTION_A: return redirect_a(e);
  case REDIRECTION_E: return redirect_e(e);
  case REDIRECTION_EO : return redirect_eo(e);
    break;
  case PIPE: return pipe_expr(e);
  case SEQUENCE: return sequence(e);
  case SEQUENCE_ET: return sequence_and(e);
  case SEQUENCE_OU: return sequence_or(e);
  default :
	break;
  }

  return 1;
}
