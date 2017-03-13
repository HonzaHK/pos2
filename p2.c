#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>


typedef struct {
	bool used;
	char *fileName;
	FILE *filePtr;
} clredir_t;

void clparse(char *input, char **cmd){
     while(*input!='\0'){
          while(*input==' ' || *input=='\t' || *input=='\n'){
               *input++='\0'; //skip whitespaces
           }
          *cmd++=input; //save start pos
          while(*input!='\0' && *input!=' ' && *input!='\t' && *input!='\n'){
          	input++; //read out word
          }
     }
}

void clexec(char **cmd, clredir_t inFile, clredir_t outFile, bool isBg){
    pid_t  pid;
    int    status;

	if ((pid = fork()) < 0) {
	  printf("fork error\n");
	  exit(1);
	}
	else if (pid == 0) { //child
		//printf("# child pid is %d\n", getpid());
	    // int stdinCopy = dup(0);
	    // int stdoutCopy = dup(1);
	    if(inFile.used){
		    inFile.filePtr = fopen(inFile.fileName, "r");
			if (inFile.filePtr==NULL) { printf("inFile error\n");return ; }
			if (dup2(fileno(inFile.filePtr),fileno(stdin))<0) { printf("inDup error");return ;}
			fclose(inFile.filePtr);
		}
	    if(outFile.used){
		    outFile.filePtr = fopen(outFile.fileName, "w");
			if (outFile.filePtr==NULL) { printf("outFile error\n");return ; }
			if (dup2(fileno(outFile.filePtr),fileno(stdout))<0) { printf("outDup error");return ;}
			fclose(outFile.filePtr);
		}

		if (execvp(*cmd, cmd) < 0) {
		   printf("execvp error\n");
		   exit(1);
		}
	}
	else { //parrent
		//printf("# parent pid is %d\n", getpid());
		if(isBg){
			//setpgid(0,0);
		}
		else{
			while (wait(&status) != pid);
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
	char input[513];
	char *words[64]; //input without whitespaces
	char *cmd[64]; //shell command with args
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
		for(unsigned int i=0; i<sizeof(words); i++){
			words[i] = NULL;
		}
		for(unsigned int i=0; i<sizeof(cmd); i++){
			cmd[i] = NULL;
		}
		inFile.used = outFile.used = false;
		inFile.fileName = outFile.fileName = NULL;
		inFile.filePtr = outFile.filePtr = NULL;
		isBg = false;
		//----------------------------------------------

		printf("$ "); //prompt
		fflush(stdout);
		read(0, input, sizeof(input));
		input[strlen(input)-1] = '\0'; //remove last char (newline)
		if(strcmp(input,"exit")==0){ break;}

		clparse(input,words);

		for(int i=0; words[i]!=NULL; i++){
			if(strcmp(words[i],"<")==0){
				i++; //move to filename
				if(words[i]==NULL){ return 1; } //filename missing
				inFile.used = true;
				inFile.fileName = words[i];
			}
			else if(strcmp(words[i],">")==0){
				i++; //move to filename
				if(words[i]==NULL){ return 1; } //fileName missing
				outFile.used = true;
				outFile.fileName = words[i];
			}
			else if(strcmp(words[i],"&")==0){
				isBg = true;
			}
			else{
				cmd[i] = words[i];
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