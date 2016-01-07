#include "Commandes_Internes.h"
#include "Evaluation.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <readline/history.h>
#include <signal.h>
// Fonctions internes

int kill2(int argc, char **argv)
{
		int pid = atoi(argv[2]);
		int sig = - atoi(argv[1]);
		return kill(pid,sig);
}

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
