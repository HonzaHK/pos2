//FIT VUTBR - POS - project 2
//JAN KUBIS / xkubis13
#define _POSIX_C_SOURCE 199506L
//#define _REENTRANT //gcc yells redefinition

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

#define BUF_SIZE 512
#define TOK_BUF_SIZE 1024
#define TOKEN_CNT 64
#define CMD_CNT 64

struct sigaction sa_int;

typedef struct { //critical section lock
    pthread_cond_t cond;
    pthread_mutex_t mutex;
} cs_lock_t;
cs_lock_t csLock;

typedef struct { //critical section data
    char buffer[BUF_SIZE+1];
    bool isExec;
    volatile bool isExit;
} cs_data_t;
cs_data_t csData;

typedef struct { //structure for command-line file redirect
	bool used;
	char *fileName;
	int filePtr;
} clredir_t;

void cl_tokenize(char *buffer, char *tokenBuffer, char** tokens){
	
	int cnt = 0;
	int pos = 0, start_pos = 0;

	while(*buffer!='\0'){
		while(*buffer==' ' || *buffer=='\t' || *buffer=='\n'){
			buffer++; //skip whitespaces
		}

		start_pos = pos;
		if(*buffer=='>' || *buffer=='<' || *buffer=='&'){
			tokenBuffer[pos] = *buffer;
			pos++;
			tokenBuffer[pos] = '\0';
			pos++;
			tokens[cnt] = &tokenBuffer[start_pos];
			cnt++;
			buffer++;
		}
		else{
			while(*buffer!='\0' && *buffer!=' ' && *buffer!='\t' && *buffer!='\n' && *buffer!='>' && *buffer!='<' && *buffer!='&'){
				tokenBuffer[pos] = *buffer;
				pos++;
				buffer++; //read out word
			}
			tokenBuffer[pos] = '\0';
			pos++;
			tokens[cnt] = &tokenBuffer[start_pos];
			cnt++;
		}

	}

}


void sigintHandler(){

}

void sigchldHandler(){
	
    pid_t pid;
    int   status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) //formerly !=-1
    {
    	printf("# process %d done (parent %d)\n",pid, getpid());
        //unregister_child(pid, status);
    }
}

void clexec(char **cmd, clredir_t inFile, clredir_t outFile, bool isBg){
	
	if(isBg){
		sa_int.sa_handler = SIG_IGN;
		sigaction(SIGINT, &sa_int, NULL);
	}
	else{
		sa_int.sa_handler = sigintHandler;
		sigaction(SIGINT, &sa_int, NULL);
	}

	pid_t pid;
	if ((pid = fork()) < 0) {
		printf("fork error\n");
		exit(1);
	}
	else if (pid == 0) { //child
		// printf("# child pid is %d\n", getpid());
		// int stdinCopy = dup(0);
		// int stdoutCopy = dup(1);
		if(inFile.used){
			inFile.filePtr = open(inFile.fileName, O_RDONLY);
			if (inFile.filePtr==-1) { printf("inFile error\n");return ; }
			if (dup2(inFile.filePtr,fileno(stdin))<0) { printf("inDup error");return ;}
			close(inFile.filePtr);
		}
		if(outFile.used){
			outFile.filePtr = open(outFile.fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			if (outFile.filePtr==-1) { printf("outFile error\n");return ; }
			if (dup2(outFile.filePtr,fileno(stdout))<0) { printf("outDup error");return ;}
			close(outFile.filePtr);
		}
		if (execvp(cmd[0], cmd) < 0) {
			printf("# ERR_EXECVP\n");
		}
		pthread_exit(0);
	}
	else { //parrent
		int status;
		//printf("# parent pid is %d\n", getpid());
		//setpgid(pid,pid);
		if(isBg){

		}
		else{
			waitpid(pid,&status,0);
		}
	}

	return;
}

void *thr_exec_func(){

	char tokenBuffer[TOK_BUF_SIZE];
	char *tokens[TOKEN_CNT] = {NULL}; //array with input tokens
	char *cmd[CMD_CNT] = {NULL}; //shell command with args
	clredir_t inFile,outFile; //input/output redir
	bool isBg; //background process flag
	
	while(true){
		pthread_mutex_lock(&csLock.mutex);
		while(!csData.isExec){
			pthread_cond_wait(&csLock.cond,&csLock.mutex);
		}
		if(csData.isExit){
			break;
		}
		//init memory -----------------------------------
		for(unsigned int i=0; i<TOK_BUF_SIZE; i++){
			tokenBuffer[i] = '\0';
		}
		for(unsigned int i=0; i<TOKEN_CNT; i++){
			tokens[i] = NULL;
		}
		for(unsigned int i=0; i<CMD_CNT; i++){
			cmd[i] = NULL;
		}
		inFile.used = outFile.used = false;
		inFile.fileName = outFile.fileName = NULL;
		inFile.filePtr = outFile.filePtr = -1;
		isBg = false;
		int cmdCnt = 0;
		//-----------------------------------------------
		cl_tokenize(csData.buffer,tokenBuffer,tokens); //parse by lexems (<,>,&,whitespace,else)

		for(int i=0; i<TOKEN_CNT && tokens[i]!=NULL; i++){
			if(strcmp(tokens[i],"<")==0){
				i++; //move to filename
				if(tokens[i]==NULL){ return NULL; } //filename missing
				inFile.used = true;
				inFile.fileName = tokens[i];
			}
			else if(strcmp(tokens[i],">")==0){
				i++; //move to filename
				if(tokens[i]==NULL){ return NULL; } //fileName missing
				outFile.used = true;
				outFile.fileName = tokens[i];
			}
			else if(strcmp(tokens[i],"&")==0){
				isBg = true;
			}
			else{
				cmd[cmdCnt++] = tokens[i];
			}
		}
		
		if(cmd[0]){
			clexec(cmd, inFile, outFile, isBg);
		}

		csData.isExec = false;
		pthread_cond_broadcast(&csLock.cond);
		pthread_mutex_unlock(&csLock.mutex);
	}

	return NULL;
}

void *thr_read_func(){
	while(!csData.isExit){
		pthread_mutex_lock(&csLock.mutex);
		while(csData.isExec){
			pthread_cond_wait(&csLock.cond,&csLock.mutex);
		}
		csData.isExec = true; //if not exit, then 100% execution
		//init buffer -----------------------------------
		for(unsigned int i=0; i<BUF_SIZE; i++){ 
			csData.buffer[i] = '\0';
		}
		//-----------------------------------------------
		printf("$ "); //prompt
		fflush(stdout);
		read(0, csData.buffer, sizeof(csData.buffer));
		if(csData.buffer[0]=='\0'){ csData.isExit=true;}
		csData.buffer[strlen(csData.buffer)-1] = '\0'; //remove last char (newline)
		if(strcmp(csData.buffer,"exit")==0){csData.isExit = true;}
		

		pthread_cond_broadcast(&csLock.cond);
		pthread_mutex_unlock(&csLock.mutex);
	}

	return NULL;
}


int main(){
	
	struct sigaction sa_chld;
	memset(&sa_chld, 0, sizeof(sa_chld));
	sa_chld.sa_handler = sigchldHandler;
	sigaction(SIGCHLD, &sa_chld, NULL);
	
	memset(&sa_int, 0, sizeof(sa_int));
	sa_int.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sa_int, NULL);

	pthread_mutex_init(&csLock.mutex,NULL);
	csData.isExec = false;
	csData.isExit = false;

	pthread_t thr_read, thr_exec;
	if(pthread_create(&thr_read, NULL, &thr_read_func, NULL)) {
			fprintf(stderr, "Error creating thread\n");
			return 1;
	}
	if(pthread_create(&thr_exec, NULL, &thr_exec_func, NULL)) {
			fprintf(stderr, "Error creating thread\n");
			return 1;
	}

	pthread_join(thr_read,NULL);
	pthread_join(thr_exec,NULL);

	return 0;
}