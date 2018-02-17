#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<fcntl.h>


#define delimiter " \t\n\a\r"   //delimiters for parsing commands
#define bufsize 128             //buffer size to hold tokens

int pipes_no = 0;               //global variables used in pipe
int indices[20];


/**** GETTING INPUT LINE ****/


char * get_line()
{
    static char string[1024];

    printf("SISH:> ");

    if(fgets(string, 1024, stdin) == NULL)        // if EOF or ctrl + d is entered, return 0
        return 0;

    return string;

}


/**** PARSING THE LINE ****/


char ** parse(char * str)
{
    char *token;
    char **tokens;
    char input[64], output[64];
    char *sub_token;

    int k = -1;   //for storing the index of pipes
    int i = 0;    //index for the tokenization
    int amp = 0;  //flag for background process

    tokens = malloc(bufsize * sizeof(char*));      // allocate memory for tokens

label:

    token = strtok(str, delimiter);

    if (!token)                                   // if nothing but ENTER key is pressed, keep looping
    {
        str = get_line();

        if (!str)                                 // if EOF or ctrl + d is entered, exit
            exit(0);

        goto label;
    }

    while(token)
    {

      if (strchr(token, '&'))                // if & is attached to a command to run in background
      {
            token[strlen(token) - 1] = '\0';         //replace & with '\0' to terminate the string
            amp = 1;                                 // set background process flag to 1
      }

      if(strcmp(token, "|") == 0)
      {
            pipes_no++;   // To know the number of pipes in the command
            k++;
            indices[k] = i;       //pipes indices array
            tokens[i] = NULL;

            i++;
            token = strtok(NULL, delimiter);
      }

      else if (strchr(token, '<') && strlen(token) != 1)    // if command is like cat<file, i.e. no spaces
      {
            sub_token = strtok(str, "<");                   //take first command

            while(sub_token){

                tokens[i] = sub_token;                      //put first command in tokens array
                tokens[i + 1] = "<";                        //put < next
                i += 2;
                sub_token = strtok(NULL, "<");

            }

            tokens[i] = NULL;                      //remove duplicate commands
            tokens[i - 1] = NULL;                  //remove duplicate commands

      } else if(strchr(token, '>') && strlen(token) != 1)  // if command is like cmd>file, i.e. no spaces
      {
            sub_token = strtok(str, ">");                  //take first command

            while(sub_token){

                tokens[i] = sub_token;                     //put first command in tokesn array
                tokens[i + 1] = ">";                       //put > next
                i += 2;
                sub_token = strtok(NULL, ">");

            }

            tokens[i] = NULL;                     //remove duplicate commands
            tokens[i - 1] = NULL;                 //remove duplicate commands
      }

      else{                                      //else all commands are separated with white spaces

      tokens[i] = token;
      i++;
      token = strtok(NULL, delimiter);
      }
    }


    if (amp == 1)                               //if background process flag is 1
    {
        tokens[i] = "&";                        //attach & at the end of tokens array
        tokens[i + 1] = NULL;

    } else {

        tokens[i] = NULL;
    }

    return tokens;
}


/**** EXECUTING COMMANDS ****/


char execute(char** str)
{

    int i;
    int in_flag = 0;
    int out_flag = 0;

    char input[64], output[64];

    for (i = 0; str[i] != '\0'; i++)    //scaning parsed string for < and >
    {

        if(strcmp(str[i], "<") == 0)    //if < is found
        {
            str[i] = NULL;              //replace it with NULL
            strcpy(input, str[i + 1]);  //copy file name to input
            in_flag = 1;
        }

        else if(strcmp(str[i], ">") == 0)    //if > is found
        {
            str[i] = NULL;              //replace it with NULL
            strcpy(output, str[i + 1]); //copy file name to outpu
            out_flag = 1;
        }
    }

    if (in_flag)                       //if < is found
    {
        int fd_in;                     //file descriptor

        fd_in = open(input, O_RDONLY, 0);       //open file

        if (fd_in < 0){                     // if file not exist
            perror("Failed");
            exit(0);
        }
        dup2(fd_in, STDIN_FILENO);     //makes a copy of fd_in to the file descriptor STDIN_FILENO
        close(fd_in);                  //close file descriptor

    }

    if (out_flag)                      //if > is found
    {
        int fd_out;                       //file descriptor
        fd_out = creat(output, 0644);     //if file doesn't exist, it will be created
        dup2(fd_out, STDOUT_FILENO);      //makes a copy of fd_out to the file descriptor STDOUT_FILENO
        close(fd_out);                  //close file descriptor

    }

    if (strcmp(str[0],"cd") == 0)      //if changing directory is required
    {
        int cdflag;
		cdflag = chdir(str[1]);        //change directory

		if(cdflag == 0){

			printf("Changing Directory Succeeded\n");
			return 0;

		} else{

			perror("ERROR");
			return 0;
		}
    }

    execvp(*str, str);              //execute command here
    perror("Failed to exec");
    exit(0);

}

/**** CHECK FOR BACKGROUND PROCESS ****/


int back_process(char ** str)
{
    int background = 0;
    int i;

    for (i = 0; str[i] != '\0'; i++)   //scan parsed string
    {

        if (strcmp(str[i], "&") == 0)  //if & is found
        {
            str[i] = NULL;             //replace it with NULL
            background = 1;            //background flag is 1
        }
    }

    return background;

}

/**** EXECUTE PIPES ****/


void loop_pipe(char ***cmd)
{
    int fd_in = 0;
    int p[2];       //array for (read/write) to/from the pipe

    pid_t pid;      //identifying the id of the process


    while( *cmd != NULL)     //walking through the command
    {
      pipe(p);               //Creating the pipe

      if ((pid = fork()) == -1)     //create new process which is needed for writing or reading in/from the pipe
      {
          exit(EXIT_FAILURE);
      }

      else if (pid == 0)    //The Child
      {
          dup2(fd_in, 0);          // change the input according to the old one

          if (*(cmd + 1) != NULL)
            dup2(p[1], 1);         //Writing from the pipe

          close(p[0]);             //So close the reading side

          execute(*cmd);      //Executing the command
          exit(EXIT_FAILURE);

      } else
        {              //The Parent
          wait(NULL);
          close(p[1]);   //close the writing side to read
          fd_in = p[0]; //save the input for the next command
          cmd++;
        }
    }

}

/**** BEGIN EXECUTING SHELL FROM HERE ****/

int main()
{
    int pflag = 0;
    int status = 0;
    int i, ret;

    char *str;           //input string declaration
    char **parsed;       //parsed string declaration
    char *comm1[100];    //the first command for PIPE case
    char *comm2[100];    //the second command after 1'st pipe
    char *comm3[100];    //Third command after 2'nd pipe
    char *comm4[100];    //The fourth command after the 3'rd pipe

    pid_t pid, pid0,pid1, pid2, pid3;

    str = get_line();               //call function to get input command

    if (!str)
        exit(0);

    parsed = parse(str);            //call function to parse the command
    pflag = back_process(parsed);   //check wether a background process is required or not


	while(strcmp(*parsed, "exit") != 0){ /* while loop for repeating the shell to input a command */

        if(pipes_no > 0)  //case of there is pipes in the command
        {

            if(pipes_no == 1){       // case of there is one pipe

				int i = 0;

				while(i <= indices[0]){       // loop through the command afrter parsing until the the index of the first pipe *

					comm1[i] = parsed[i];
					i++;                      //This index for going through the parsed command
				}

				int j = 0;                   // for going through the command before each pipe

				while(i < sizeof(parsed)){  // loop through the command after the pipe and put it in comm2 array
					comm2[j] = parsed[i];
					i++;
					j++;
				}

				comm2[j] = NULL;         // each comm should ends with NULL character

				char **buf[] = {comm1, comm2, NULL};
				loop_pipe(buf);
			}

			if(pipes_no == 2){  //case of two pipes

				int i = 0;

				while(i <= indices[0]){
					comm1[i] = parsed[i];
					i++;
				}

				int j = 0;

				while(i < indices[1]){
					comm2[j] = parsed[i];
					i++;
					j++;
				}

				comm2[j] = NULL;

				j=0;

				while(i < sizeof(parsed)){
					comm3[j] = parsed[i+1];
					i++;
					j++;
				}

				comm3[j] = NULL;

				char **buf[] = {comm1, comm2, comm3, NULL};
				loop_pipe(buf);
			}

			if(pipes_no == 3){   //case of three pipes

				int i = 0;

				while(i <= indices[0]){
					comm1[i] = parsed[i];
					i++;
				}

				int j = 0;

				while(i < indices[1]){
					comm2[j] = parsed[i];
					i++;
					j++;
				}
				comm2[j] = NULL;

				j=0;

				while(i < indices[2]){
					comm3[j] = parsed[i+1];
					i++;
					j++;
				}

				comm3[j] = NULL;

				j=0;

				while(i < sizeof(parsed)){
					comm4[j] = parsed[i+1];
					i++;
					j++;
				}

				comm4[j] = NULL;

				char **buf[] = {comm1, comm2, comm3,comm4, NULL};  // The full command to pass to the pipe function to execute
				loop_pipe(buf);
			}

			pipes_no = 0;            // return back the number of pipes to zero because of looping

			str = get_line();        //get back to insert a command

			if (!str)
                exit(0);

			parsed = parse(str);     //Parsing again


        } else {

            pid = fork();               //create a process

            if(pid < 0){

                printf("Fork Failed");

            } else if (pid == 0){       //child

                    if (pflag){         //if background process is required

                        fclose(stdin);              //close child's stdin
                        fopen("/dev/null", "r");    //open a new stdin that is always empty
                        execute(parsed);

                    } else{                          //If background process is not required

                        execute(parsed);             //execute command normally

                    }

            } else{                      //parent process waiting on the child

                if (pflag){
                    printf("Starting background process %d\n", pid);

                }else{
                    waitpid(pid, &status, 0);  //waiting on child process
                }

            }

            str = get_line();                  //get a new command
            if (!str)
                exit(0);

            parsed = parse(str);               //parse it
            pflag = back_process(parsed);      //check for background process

        }
    }

}
