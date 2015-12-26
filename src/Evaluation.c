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

int bg = 0;

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

void echo2(char *buf)
{
	if( ( buf[0] == '"'  && buf[strlen(buf) - 1] == '"' ) || ( buf[0] == '\'' && buf[strlen(buf) - 1] == '"' ) )
	{
		char *bufFinal;
		strcpy(bufFinal, buf + 1);
		printf("%s prout\n", bufFinal);
	} 
	else
	{
		printf("%s prout2 %c %c\n", buf, buf[0], buf[strlen(buf) - 1]);
	}
}

void date2()
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
}

void executer_simple(Expression *e, int bg)
{
	if(strcmp(e->arguments[0], "exit") == 0)
		exit2();
	else if(strcmp(e->arguments[0], "pwd") == 0)
		pwd2();
	else if(strcmp(e->arguments[0], "hostname") == 0)
		hostname2();
	else if(strcmp(e->arguments[0], "echo") == 0)
	{
		printf("echo2 %s\n", e->arguments[1]);
		echo2(e->arguments[1]);			
	}
	else if(strcmp(e->arguments[0], "date") == 0)
		date2();
	else
	{	
		int pid = fork();

		if(pid == 0)
			execvp(e->arguments[0], e->arguments);	
		else if(pid > 0)
			if(!bg)
				wait(NULL);
		else
			printf("Erreur\n");
	}
}


int evaluer_expr(Expression *e)
{

  if (e == NULL) return 0;

  switch(e->type){

  case VIDE :
    break ;
    
  case SIMPLE :	
	executer_simple(e, bg);
	bg = 0;
    break;

  case REDIRECTION_I: 	
  case REDIRECTION_O: 	
  case REDIRECTION_A: 	
  case REDIRECTION_E: 	
  case REDIRECTION_EO :
    break;
  case BG:
  	bg = 1;
  	evaluer_expr(e->gauche);
    break;
  default :
	break;
  }

  return 1;
}
