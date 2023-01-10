#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <math.h>

#define BUFSIZE 1096

void exeCommande(char **buf, ssize_t commande_size);
void writeReturn(char *str1, int code, int time_ms);
char** separeCommande(char *cmd);

int argc;

int main() {
	
	char welcome[BUFSIZE] = "Bienvenue dans le Shell ENSEA.\nPour quitter, tapez 'exit'.\nenseash ";
	char buf[BUFSIZE];
	char **list_cmd;
	ssize_t commande_size;
	
	write(1, welcome, strlen(welcome)); 
	
	while(1) { // Lecture des commandes
		write(1, "% ", strlen("% "));
		memset(buf, 0, BUFSIZE); // clear buffer
		argc = 0;
		if ((commande_size = read(0, buf, BUFSIZE)) == -1) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		list_cmd = separeCommande(buf);
		exeCommande(list_cmd, commande_size);
	}
}

void exeCommande(char **buf, ssize_t commande_size) {
	int pid, status;
	char msg_out[BUFSIZE] = "enseash [";
	struct timespec start, stop;
	int time_exe_ns, time_exe_s, time_exe_ms;
	
	if (strcmp("exit", buf[0]) == 0 || commande_size == 0) {
		write(1, "Bye bye...\n", strlen("Bye bye...\n"));
		exit(EXIT_FAILURE);
	} 
	
	pid = fork();

	if (pid != 0) { // Le père attends la fin de l'execution de la commande dans le fils.
		if (clock_gettime(CLOCK_REALTIME, &start) == -1) { // Recupération du temps avant l'execution d'une commande
			perror("clock gettime");
			exit(EXIT_FAILURE);
		}
		
		wait(&status);
		
		if (clock_gettime(CLOCK_REALTIME, &stop) == -1) { // Temps à la fin de l'execution
			perror("clock gettime");
			exit(EXIT_FAILURE);
		}
		
		// ns et s -> ms
		time_exe_ns = stop.tv_nsec - start.tv_nsec;
		time_exe_s = stop.tv_sec - start.tv_sec;
		time_exe_ms = time_exe_s * 1000 + time_exe_ns * pow(10, -6);
		
		// Codes de sorties
		if (WIFEXITED(status)) {
			strcat(msg_out, "exit:");
			writeReturn(msg_out, WEXITSTATUS(status), time_exe_ms);
		} else if (WIFSIGNALED(status)) {
			strcpy(msg_out, "sign:");
			writeReturn(msg_out, WTERMSIG(status), time_exe_ms);
		}
	} else { // Fils pour l'execution des commandes
		char **new_buf = malloc(argc * sizeof(char*));
		int file;
		
		for (int i = 0; i < argc; i++) {
			// Gère la redirection des commandes
			if (strcmp(buf[i], ">") == 0) {
				file = open(buf[i + 1], O_WRONLY | O_TRUNC);
				if (file == -1) {
					perror("file");
					exit(EXIT_FAILURE);
				}
				if (dup2(file, STDOUT_FILENO) == -1) {
					perror("dup2");
					exit(EXIT_FAILURE);
				}
				break;
			} else if (strcmp(buf[i], "<") == 0) {
				file = open(buf[i + 1], O_RDONLY);
				if (file == -1) {
					perror("file");
					exit(EXIT_FAILURE);
				}
				if (dup2(file, STDIN_FILENO) == -1) {
					perror("dup2");
					exit(EXIT_FAILURE);
				}
				break;
			}
			/* les arg de la commandes avant les symboles < ou >
			   sont copiés dans un nouveau tableau pour être éxécutés */
			new_buf[i] = malloc(strlen(buf[i]));
			strcpy(new_buf[i], buf[i]);
		}
		close(file);
		execvp(new_buf[0], new_buf); 
		exit(EXIT_SUCCESS);
	}
}

void writeReturn(char *str1, int code, int time_ms) {
	char code_commande[BUFSIZE];
	
	sprintf(code_commande, "%d", code);
	strcat(str1, code_commande);
	strcat(str1, "|");
	sprintf(code_commande, "%d", time_ms);
	strcat(str1, code_commande);
	strcat(str1, "ms] ");
	write(1, str1, strlen(str1));
}

char** separeCommande(char *cmd) {
	char **argv;
	argv = malloc(20*sizeof(char*));
	int i= 0;
	char *tmp = strtok(cmd, " ");
	
	while (tmp != NULL) {
		argv[i] = malloc(20);
		strcpy(argv[i], tmp);
		tmp = strtok(NULL, " "); 
		i++;
	}
	argv[i-1][strlen(argv[i - 1]) - 1] = '\0'; // supprime le cracatere enter sur le dernier arg
	argc = i;
	return argv;
}
