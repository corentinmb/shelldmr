#include "Shell.h"
#include "Evaluation.h"
#include "Commandes_Internes.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <readline/history.h>

int bg = 0;
int indice = 0;

struct distant_shell
{
	int tube[2];
	char hostname[40];
};

struct distant_shell tab_shell[20];

// Fonctions internes



void exit2()
{
	fflush(stdin);
	close(0);
	fflush(stdout);
	close(1);
	fflush(stderr);
	close(2);
	
	kill(getpid(), SIGINT);
}

int pwd2()
{
	char buf[256];
	getcwd(buf, sizeof(buf));
	
	if(buf == NULL)
		return -1;
	else
		{
			printf("%s\n", buf);
			return 0;
		}
}

int hostname2()
{
	FILE *fd = fopen("/etc/hostname", "r");
	char *buf;
	size_t len = 0;
	
	if(getline(&buf, &len, fd) == -1)
		return -1;
	else
	{
		printf("%s", buf);
		return 0;	
	}
}

// problème lors de l'input utilisateur, les "" ou '' sont enlevé par le code du prof

int echo2(char *buf)
{
	if( ( buf[0] == '"'  && buf[strlen(buf) - 1] == '"' ) || ( buf[0] == '\'' && buf[strlen(buf) - 1] == '"' ) )
	{
		char *bufFinal;
		strcpy(bufFinal, buf + 1);
		//printf("%s prout\n", bufFinal);
	} 
	else
	{
		printf("%s\n", buf);
	}
	return 0;
}

int date2()
{
	char *tabDays[] = {"dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"};
	char *tabMonths[] = {"janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre", "otobre", "novembre", "décembre"};
	char hour[2], min[2], sec[2];
	
	time_t rawtime;
   	time( &rawtime );

   	struct tm tm = *localtime( &rawtime );
	
	// ces conditions permettent de rajouter le 0 si la valeur est < à 10
	if(tm.tm_hour < 10)
		sprintf(hour, "0%d", tm.tm_hour);
	else
		sprintf(hour, "%d", tm.tm_hour);
	
	if(tm.tm_min < 10)
		sprintf(min, "0%d", tm.tm_min);
	else
		sprintf(min, "%d", tm.tm_min);
		
	if(tm.tm_sec < 10)
		sprintf(sec, "0%d", tm.tm_sec);
	else
		sprintf(sec, "%d", tm.tm_sec);	
	
	printf("%s %d %s %d, %s:%s:%s (UTC+0100)\n", tabDays[tm.tm_wday], tm.tm_mday, tabMonths[tm.tm_mon], tm.tm_year + 1900, hour, min, sec);
    return 0;
}

int history2()
{
	using_history();
	HISTORY_STATE *histoState = history_get_history_state();	
	
	int i;
	for(i = 0; i < histoState->length; i++)
		printf("%d %s\n", i, (*(histoState->entries + i))->line);
    return 0;
}

int cd2(char *path)
{
	if(chdir(path))
		return -1;
	else
		return 0;
}

// Fin des fonction internes



int remoteAdd(char *machine)
{
	pipe(tab_shell[indice].tube);
	
	if(!fork())
	{
		dup2(tab_shell[indice].tube[0], 0);	
		execlp("ssh", "ssh", machine, "bash", NULL);
		return -1;
	}

	strcpy(tab_shell[indice++].hostname, machine);
	return 0;
}

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

int remoteCmd(char *host, char * cmd)
{
	int i, found = 0;
	for(i = 0; i < indice; i++)	
	{
		if(strcmp(tab_shell[i].hostname, host) == 0)
		{
			sprintf(cmd, "%s\n", cmd);
			write(tab_shell[i].tube[1], cmd, sizeof(cmd));
			found =  1;
		}
	}
	
	if(!found)
		printf("Erreur machine invalide\n");
	
	return 0;
}

int remoteRemove()
{	
	int i;
	for(i = 0; i < indice; i++)
	{
		write(tab_shell[i].tube[1], "exit\n", sizeof("exit\n")); 
		close(tab_shell[i].tube[1]);
		close(tab_shell[i].tube[0]);
	}
		
	indice = 0;	
	return 0;
}

int executer_simple(Expression *e, int bg)
{
	if(strcmp(e->arguments[0], "exit") == 0)
		exit2();
	else if(strcmp(e->arguments[0], "pwd") == 0)
		return pwd2();
	else if(strcmp(e->arguments[0], "hostname") == 0)
		return hostname2();
	else if(strcmp(e->arguments[0], "echo") == 0)
	{
		return echo2(e->arguments[1]);			
	}
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
			if(remoteAdd(e->arguments[2]))
				printf("Erreur, impossible de se connecter\n");
				return 2;
		}
		else if(strcmp(e->arguments[1], "remove") == 0)
			return remoteRemove();			
		else
		{
			if(remoteCmd(e->arguments[1], e->arguments[2]))
				printf("Erreur, hote ou commande invalide\n");
				return 3;
		}
	}
	else
	{	
		int pid = fork();
        
		if(pid == 0)
			execvp(e->arguments[0], e->arguments);	
		else if(pid > 0)
		{
		    int status;
			if(!bg)
			{
				waitpid(pid, &status, 0);
				return (WIFEXITED(status)? WEXITSTATUS(status) : -1);
		    }
		}
		else
			printf("Erreur\n");
			return -1;
	}
}


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


// Pour rendre le code plus propre à rajouter une fois que les fonctions seront terminées

int evaluer_expr(Expression *e)
{

  if (e == NULL) return 0;

  switch(e->type){

  case VIDE :
    break ;
    
  case SIMPLE :	
	bg = 0;
	return executer_simple(e, bg);
	break;
 case BG:
  	bg = 1;
  	return evaluer_expr(e->gauche);
    break;
 
  case REDIRECTION_I: return redirect_i(e);	
  case REDIRECTION_O: return redirect_o(e);	
  case REDIRECTION_A: return redirect_a(e);	
  case REDIRECTION_E: return redirect_e(e);	
  case REDIRECTION_EO : return redirect_eo(e);
    break;
  default :
	break;
  }

  return 1;
}
