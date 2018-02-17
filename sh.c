#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include<sys/stat.h>
#include<sys/types.h>

char *tokens[100];
char *read_command(){
	int bytes_read;
	size_t nbytes = 1024;
	char *cmd;
 	char *my_string;

	printf("sish:> ");

	/* These 2 lines are the heart of the program. */
	my_string = (char *) malloc (nbytes + 1);
	bytes_read = getline (&my_string, &nbytes, stdin);

	if (bytes_read == -1){
		puts ("ERROR!");
	}else{
		//puts ("You typed:");
		cmd = my_string;
	}
	return cmd;
}

void parse_cmd(){
	char *token;
	char line[1024];
	int i = 0;
	int j = 0;
	int ret;
	DIR *dir;
	struct dirent *ent;

	char *c = read_command();
	while(*c != '\0'){
		line[i] = *c;
		i++;
		c++;
	}
	token = strtok(line," ");
	while(token != NULL){
		tokens[j] = token;
		token = strtok(NULL," ");
		j++;
	}
	//printf("%s\n",tokens[2]);
	/*int r=0;
	while(strcmp(tokens[r],"\0")!=0){
		printf("%s",tokens[r]);
		r++;
	}*/

	//COMMANDS EXECUTING.......
	if(strcmp(tokens[0],"ls") == 0){
		if ((dir = opendir ("./")) != NULL) {
	  		printf(" => All the files and directories within directory : \n");
	  		while ((ent = readdir (dir)) != NULL) {
	    			printf ("     %s\n", ent->d_name);
	  		}
	  		closedir (dir);
		}else{
			perror("--!!!--");
		}
	}else if(strcmp(tokens[0],"cd") == 0){
		int ret;
		ret = chdir(tokens[1]);
		if(ret == 0){
			printf("Changing Directory Succeeded\n");
		}else{
			perror("chdir FAILED!!");
		}
	}else if(strcmp(tokens[0],"pwd") == 0){
		char buff[256];
		if(getcwd(buff, sizeof(buff)) != NULL){
			printf("%s\n",buff);
		}else{
			perror("PWD cmd FAILED!!");
		}
	}else if(strcmp(tokens[0],"mkdir") == 0){
		mode_t m = umask(0);
		int mkd = mkdir(tokens[1], S_IRWXU | S_IRWXG | S_IRWXO);
		umask(m);
		if(mkd == 0){
			printf("Making Dir. Succeeded\n");
		}else{
			perror("MKDIR FAILED!!");
		}
	}else if(strcmp(tokens[0],"rm") == 0){
		if(unlink(tokens[1]) == 0){
			printf("The file REMOVED\n");
		}else{
			perror("The file hasn't been removed");
		}
	}

}



int main(){

	parse_cmd();
	while(strcmp(tokens[0], "exit") != 0){
		parse_cmd();
		/*if(tokens[0]!=NULL){
			cmd_exec();
		}*/
	}
	return 0;
}











