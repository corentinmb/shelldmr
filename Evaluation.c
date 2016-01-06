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
FILE *remShellTab[20];
char *pcName[20];

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
		//printf("%s prout\n", bufFinal);
	} 
	else
	{
		printf("%s\n", buf);
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

void history2()
{
	using_history();
	HISTORY_STATE *histoState = history_get_history_state();	
	
	int i;
	for(i = 0; i < histoState->length; i++)
		printf("%d %s\n", i, (*(histoState->entries + i))->line);
}

int cd2(char *path)
{
	if(chdir(path))
		return -1;
	else
		return 0;
}

void remoteAdd(char *machine)
{
	char cmd[20];
	sprintf(cmd, "ssh %s -p 22", machine);
	
	FILE* remf = popen(cmd, "w");
	
	if(!remf)
	{
		perror("popen ssh failed");
		exit(EXIT_FAILURE);
	}
	
	remShellTab[indice] = remf;
	pcName[indice++] = machine;

	printf("%s | %d\n", pcName[indice - 1], indice - 1);
}

int remoteListe()
{
	if(indice != 0)
	{
		int i;
		for(i = 0; i < indice; i++)
			printf("%s\n", pcName[i]);

		return 0;
	}
	else
		return -1;
}

int remoteCmd(char *host, char * cmd)
{
	/*int i;
	for(i = 0; i < indice; i++)	
	{
		if(strcmp(pcName[i], host) == 0)
		{
			//printf("trouvé : %s\n", pcName[i]);
			char test[30];
			sprintf(test, "%s > test.txt", cmd);
			fprintf(remShellTab[i], test);
		}
	}*/

	// code qui fonctionne mais qui n'est surement pas correct
	if(!fork())		
	{
		execlp("ssh", "ssh", host, cmd, NULL);
		return -1;
	}
	else
		wait(NULL);

	return 0;
}

int remoteRemove()
{	
	if(!fork())		
	{
		//execlp("ssh", "ssh", host, "exit", NULL);
		return -1;
	}
	else
		wait(NULL);

	return 0;
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
		echo2(e->arguments[1]);			
	}
	else if(strcmp(e->arguments[0], "date") == 0)
		date2();
	else if(strcmp(e->arguments[0], "history") == 0)
		history2();
	else if(strcmp(e->arguments[0], "cd") == 0)
		cd2(e->arguments[1]);
	/*else if(strcmp(e->arguments[0], "remote") == 0)
	{		
		if(strcmp(e->arguments[1], "list") == 0)
		{
			if(remoteListe())
				printf("Erreur, aucune machine connecté en ssh\n");
			printf("test\n");
		}
		else if(strcmp(e->arguments[1], "add") == 0)			
			remoteAdd(e->arguments[2]);
		else if(strcmp(e->arguments[1], "remove") == 0)
			remoteRemove();
			
		else
			remoteCmd(e->arguments[1], e->arguments[2]);
	}*/
	else if(strcmp(e->arguments[0], "remote") == 0)
		remoteAdd("infini1");
	else if(strcmp(e->arguments[0], "liste") == 0)
	{
		if(remoteListe())
			printf("Erreur, aucune machine connecté en ssh\n");
	}
	else if(strcmp(e->arguments[0], "cmd") == 0)
	{
		if(indice != 0)
		{
			if(remoteCmd("infini1", "ls"))
				printf("Erreur, hote ou commande invalide\n");
		}
		else
			printf("Erreur, vous n'êtes connecté à aucune machine distante\n");
	}
	else if(strcmp(e->arguments[0], "remove") == 0)
	{
		remoteRemove();
	}
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

// Pour rendre le code plus propre à rajouter une fois que les fonctions seront terminées


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
