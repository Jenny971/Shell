# Shell
A simple implementation of shell.

This is a course project, to write a shell in C with some basic features.
Still trying to improve the code, and more detail might be added to README in the future.


Structures SubCommand and Command

Data structures Command represent a command line, composed of multiple sub-commands separated by pipes (“ ” character). 
Each command has an array of, at most, MAX_SUB_COMMANDS sub- commands. Another field named num_sub_commands indicates how many sub-commands are present. 
struct Command
{
struct SubCommand sub_commands[MAX_SUB_COMMANDS];
int num_sub_commands;
}; 

Each sub-command contains a field called containing the entire sub-command as a C string, as well as a null-terminated array of at most arguments (one array element is reserved for NULL). 
struct SubCommand
{ 
char *line;
char *argv[MAX_ARGS];
};

