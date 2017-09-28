#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_SUB_COMMANDS   5
#define MAX_ARGS           10
char cwd[1024];

struct SubCommand
{
    char *line;
    char *argv[MAX_ARGS];
};
struct Command
{
    struct SubCommand sub_commands[MAX_SUB_COMMANDS];
    int num_sub_commands;
    char *stdin_redirect;
    char *stdout_redirect;
    int background;
};

void ReadArgs(char *in, char **argv, int size)
{
    int args = 0;
    const char *sep = " ";
    char *token;
    token = strtok(in,sep);
    while(token != NULL && args < size - 1)
    {
        argv[args] = strdup(token);
        token = strtok(NULL,sep);
        args = args + 1;
    }
    argv[args] = NULL;
}

void PrintArgs(char **argv)
{
    int i = 0;
    while(argv[i] != NULL)
    {
        printf("argv[%d] = '%s'\n",i,argv[i]);
        i++;
    }
    printf("\n");
}


void ReadCommand(char *line, struct Command *command)
{
    command->num_sub_commands = 0;
    const char *sep = "|";
    char *token;
    line[strlen(line) - 1] = '\0';
    token = strtok(line,sep);
    while(token != NULL && command->num_sub_commands < MAX_SUB_COMMANDS)
    {
        command->sub_commands[command->num_sub_commands].line = strdup(token);
        token = strtok(NULL,sep);
        command->num_sub_commands = command->num_sub_commands + 1;
    }
    int sub;
    for(sub = 0;sub < command->num_sub_commands;sub++)
    {
        ReadArgs(command->sub_commands[sub].line , command->sub_commands[sub].argv, MAX_ARGS);
    }
}

void PrintCommand(struct Command *command)
{
    int i;
    for(i = 0;i < command->num_sub_commands;i++)
    {
        
        printf("commands %d:\n",i);

        PrintArgs(command->sub_commands[i].argv);
    }
    
    printf("Redirect stdin: %s\n",command->stdin_redirect);
    printf("Redirect stdout: %s\n",command->stdout_redirect);
    if (command->background == 1)
    {
        printf("Background: yes\n");
    }
    else
    {
        printf("Background: no\n");
    }
}

void ReadRedirectsAndBackground(struct Command *command)
{
    // read input file name
    int i,j;
    int last = command->num_sub_commands;
    for(i = last - 1;i>=0;i--)
    {
        j = 0;
        while(command->sub_commands[i].argv[j])
        {
            if (strcmp(command->sub_commands[i].argv[j], "<") == 0)
            {
                command->stdin_redirect = command->sub_commands[i].argv[j+1];
                while(command->sub_commands[i].argv[j+2])
                {
                    command->sub_commands[i].argv[j] = command->sub_commands[i].argv[j+2];
                    j++;
                }
                command->sub_commands[i].argv[j] = NULL;
            }
            j++;
        }
        if(command->stdin_redirect != NULL) break;
    }
    
    
    
    // read output file name
    for(i = last - 1;i>=0;i--)
    {
        j = 0;
        while(command->sub_commands[i].argv[j])
        {
            if (strcmp(command->sub_commands[i].argv[j], ">") == 0)
            {
                command->stdout_redirect = command->sub_commands[i].argv[j+1];
                while(command->sub_commands[i].argv[j+2])
                {
                    command->sub_commands[i].argv[j] = command->sub_commands[i].argv[j+2];
                    j++;
                }
                command->sub_commands[i].argv[j] = NULL;
            }
            j++;
        }
        if(command->stdout_redirect != NULL) break;
    }
    
    //read the background flag
    i = last - 1;
    j = 0;
    while(command->sub_commands[i].argv[j] != NULL)
    {
        j++;
    }
    j--;
    if (strcmp(command->sub_commands[i].argv[j], "&") == 0)
    {
        command->background = 1;
        command->sub_commands[i].argv[j] = NULL;
    }
    else
    {
        command->background = 0;
    }
    
    
}

int SpawnChildren(struct Command *command){
    int children[command->num_sub_commands];    
	int i;
    int fds[command->num_sub_commands-1][2];
    
	if (command->num_sub_commands >= 2){
		for (i = 0; i<command->num_sub_commands-1; i++)
        {
            pipe(fds[i]);
		}
	}
    
    for (i = 0; i<command->num_sub_commands; i++){
		children[i] = fork();
		if (children[i] < 0){
			fprintf(stderr, "fork failed\n");
			exit(1);
		}
		else if (children[i] == 0) //child process
        {
			if (command->num_sub_commands >= 2)
            {
                int j;
				for(j=0; j<command->num_sub_commands-1; j++)
                {
					if (j != i-1){
						close(fds[j][0]);
					}
					if (j != i){
						close(fds[j][1]);
					}
				}
				if (i == 0){        //the first subcommand
					close(1);
					dup(fds[i][1]);
					
				}
				else if (i == command->num_sub_commands-1)
                {
                    close(0);
					dup(fds[i-1][0]);
					
				}
				else
                {
					close(0);
					dup(fds[i - 1][0]);
					close(1);
					dup(fds[i][1]);
				}
			}
            
            if (i == 0 && command->stdin_redirect != NULL){
                close(0);
                int fd = open(command->stdin_redirect, O_RDONLY);
                if (fd < 0){
                    fprintf(stderr, "%s: File not found\n",command->stdin_redirect);
                    exit(1);
                }
            }
     
            if (i == command->num_sub_commands-1 && command->stdout_redirect != NULL)
            {
                close(1);
                int fd = open(command->stdout_redirect, O_WRONLY | O_CREAT, 0660);
                if (fd < 0)
                {
                    fprintf(stderr, "%s: Cannot create file\n",command->stdout_redirect);
                    exit(1);
                }
            }
			execvp(command->sub_commands[i].argv[0], command->sub_commands[i].argv);
            fprintf(stderr, "%s: Command not found\n",command->sub_commands[i].line);
            exit(1);
		}
	}
    for (i=0; i<command->num_sub_commands-1; i++)
    {
        close(fds[i][0]);
        close(fds[i][1]);
    }
    return children[command->num_sub_commands-1];
}



/* change cwd */
void cd(char *p)
{
    

    int change = chdir(p); //change dir
    if(change != 0)
        fprintf(stderr,"%s is nod a path,please check again \n",p);
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    perror("getcwd error");

    
}

int main()
{
	char s[200];
	struct Command *command;
    int pid_lastsub;
    
    pid_t pid_transfer;
    
    int fds_main[2];
    pipe(fds_main);
    /* save current directory */

    char cwd_save[1024];
    if (getcwd(cwd_save, sizeof(cwd_save)) == NULL)
        perror("getcwd error");
    strcpy(cwd, cwd_save);
    
	while(1)	//core loop
	{
		command = (struct Command *)malloc(sizeof(struct Command));     //?
		command->background = 0;            //0 for forground, 1 for background.
		command->stdin_redirect = NULL;
		command->stdout_redirect = NULL;
		

        
		printf("%s $ ",cwd);
		fgets(s, sizeof s, stdin);
		
		pid_transfer = waitpid(-1, NULL, WNOHANG);
		if (pid_transfer != -1)	//if has child process
		{
			fcntl(fds_main[0], F_SETFL, fcntl(fds_main[0], F_GETFL) | O_NONBLOCK);
			ssize_t r;
			do
			{
				r = read (fds_main[0], &pid_transfer, sizeof(pid_transfer));
				if (r != 0)
				{
					printf("[%d] finished\n", pid_transfer);
				}
			} while (r!= 0);
		}

        if(strcmp(s, "\n") == 0)
        {
        	free(command);
        	continue;
        }

        ReadCommand(s, command);
        ReadRedirectsAndBackground(command);
        
        //		PrintCommand(command);
        if(strcmp(command->sub_commands[0].argv[0],"cd"))	//if not cd command
        {

        	pid_t pid_main = fork();
        	if (pid_main < 0)
        	{
        		perror("fork failed");
        		exit(1);
			}
			else if (pid_main == 0)	//child
			{
				close(fds_main[0]);
				pid_lastsub = SpawnChildren(command);
				if (command->background!=1) //foreground
				{
					while (wait(NULL)>0);
				}
				else	//if background
				{
					write (fds_main[1], &pid_lastsub, sizeof(pid_lastsub));
					pid_transfer = wait(NULL);
					while (pid_transfer > 0)
					{
						write (fds_main[1], &pid_transfer, sizeof(pid_transfer));
						pid_transfer = wait(NULL);
					}
				}
				close(fds_main[1]);
				exit(1);
			}
			else if (pid_main > 0)	//parent
			{
				close(fds_main[1]);
				if (command->background != 1)
				{
					wait(NULL);
				}
				else
				{
					read (fds_main[0], &pid_lastsub, sizeof(pid_lastsub));
					printf("[%d]\n", pid_lastsub);
				}
			}
		}		
        else	//if cd command
        {
            cd(command->sub_commands[0].argv[1]);
        }
		
		free (command);
	}
    
    return 0;
}
