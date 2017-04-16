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

#define TOKEN_CNT 64
#define CMD_CNT 64

typedef struct {
	bool used;
	char *fileName;
	int filePtr;
} clredir_t;

void cl_tokenize(char *input, char** tokens){
	
	int cnt = 0;

	while(*input!='\0'){
		while(*input==' ' || *input=='\t' || *input=='\n'){
			input++; //skip whitespaces
		}

		if(*input=='>' || *input=='<' || *input=='&'){
			tokens[cnt]=malloc(sizeof(char)*2);
			tokens[cnt][0]=*input;
			tokens[cnt][1]='\0';
			input++;
			cnt++;
		}
		else{
			int length = 0;
			char *start = input;
			while(*input!='\0' && *input!=' ' && *input!='\t' && *input!='\n' && *input!='>' && *input!='<' && *input!='&'){
				input++; //read out word
				length++;
			}
			tokens[cnt]=malloc(sizeof(char)*(length+1)); //save start pos
			memcpy(tokens[cnt],start,length);
			tokens[cnt][length]='\0';
			cnt++;
		}

	}

	// for(int i= 0; i<TOKEN_CNT && tokens[i]!=NULL;i++){
	// 	printf("%s\n", tokens[i]);
	// }
}

void clexec(char **cmd, clredir_t inFile, clredir_t outFile, bool isBg){
	pid_t pid;

	if ((pid = fork()) < 0) {
		printf("fork error\n");
		exit(1);
	}
	else if (pid == 0) { //child
		//printf("# child pid is %d\n", getpid());
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
	}
	else { //parrent
		int status;
		//printf("# parent pid is %d\n", getpid());
		if(isBg){
			//setpgid(0,0);
		}
		else{
			waitpid(pid,&status,0);
		}
	}
}

void sigchldHandler(){
	
    pid_t pid;
    int   status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) //formerly !=-1
    {
    	printf("# process %d terminated by %d\n",pid, getpid());
        //unregister_child(pid, status);
    }
}

int main(){
	char input[513] = {'\0'};
	char *tokens[TOKEN_CNT] = {NULL}; //array with input tokens
	char *cmd[CMD_CNT] = {NULL}; //shell command with args
	clredir_t inFile,outFile; //input/output redir
	bool isBg; //background process flag
	
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigchldHandler;
	sigaction(SIGCHLD, &sa, NULL);
	
	while(true){
		//init memory-----------------------------------
		for(unsigned int i=0; i<sizeof(input); i++){ 
			input[i] = '\0';
		}
		for(unsigned int i=0; i<TOKEN_CNT; i++){
			if(tokens[i]!=NULL){
				free(tokens[i]);
				tokens[i]=NULL;
			}
		}
		for(unsigned int i=0; i<CMD_CNT; i++){
			cmd[i] = NULL;
		}
		inFile.used = outFile.used = false;
		inFile.fileName = outFile.fileName = NULL;
		inFile.filePtr = outFile.filePtr = -1;
		isBg = false;
		//----------------------------------------------

		printf("$ "); //prompt
		fflush(stdout);
		read(0, input, sizeof(input));
		input[strlen(input)-1] = '\0'; //remove last char (newline)
		
		if(strcmp(input,"exit")==0){
			break;
		}

		cl_tokenize(input,tokens);

		for(int i=0; tokens[i]!=NULL; i++){
			if(strcmp(tokens[i],"<")==0){
				i++; //move to filename
				if(tokens[i]==NULL){ return 1; } //filename missing
				inFile.used = true;
				inFile.fileName = tokens[i];
			}
			else if(strcmp(tokens[i],">")==0){
				i++; //move to filename
				if(tokens[i]==NULL){ return 1; } //fileName missing
				outFile.used = true;
				outFile.fileName = tokens[i];
			}
			else if(strcmp(tokens[i],"&")==0){
				isBg = true;
			}
			else{
				cmd[i] = tokens[i];
			}
		}

		// printf("cmd: ");
		// for(int i=0; cmd[i]!=NULL; i++){
		// 	printf("%s ",cmd[i] );
		// }
		// printf("\n");
		// printf("infile: %s\n", inFile.fileName);
		// printf("outfile: %s\n", outFile.fileName);
		// printf("\n");
		
		if(cmd[0]){
			clexec(cmd, inFile, outFile, isBg);
		}
	}

	return 0;
}