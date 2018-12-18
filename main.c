/***************************************************************************//**
	@file         main.c
	
	@author       Ante Bircic

	@date         Monday,  17 December 2018

	@brief        AASH (Ante Bircic SHell)

*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
Function Declarations for builtin shell commands:
*/
int mmsh_echo(char **args);
int mmsh_pwd(char **args);
int mmsh_ls(char **args);
int mmsh_mkdir(char **args);
int mmsh_cd(char **args);
int mmsh_help(char **args);
int mmsh_exit(char **args);

/*
List of builtin commands, followed by their corresponding functions.
*/
char *builtin_str[] = {
	"echo",
	"pwd",
	"ls",
	"mkdir",
	"cd",
	"help",
	"exit"
};

int(*builtin_func[]) (char **) = {
	&mmsh_echo,
	&mmsh_pwd,
	&mmsh_ls,
	&mmsh_mkdir,
	&mmsh_cd,
	&mmsh_help,
	&mmsh_exit
};

int mmsh_num_builtins() {
	return sizeof(builtin_str) / sizeof(char *);
}

/*
Builtin function implementations.
*/

/**
@brief Bultin command: echo argument.
@param args List of args.  args[0] is "echo".  args[1] is the string for echoing.
@return Always returns 1, to continue executing.
*/
int mmsh_echo(char **args)
{
	int i = 1;
	while (args[i] != NULL) {
		printf("%s ", args[i]);
		i++;
	}
	printf("\n");
	return 1;
}

/**
@brief Bultin command: print current directory.
@param args List of args.  args[0] is "cd".  args[1] is the directory.
@return Always returns 1, to continue executing.
*/
int mmsh_pwd(char **args)
{
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	printf("Current working dir: %s\n", cwd);
	return 1;
}

/**
@brief Bultin command: change directory.
@param args List of args.  args[0] is "cd".  args[1] is the directory.
@return Always returns 1, to continue executing.
*/
int mmsh_cd(char **args)
{
	if (args[1] == NULL) {
		fprintf(stderr, "mmsh: expected argument to \"cd\"\n");
	}
	else {
		if (chdir(args[1]) != 0) {
			perror("mmsh");
		}
	}
	return 1;
}

/**
@brief Bultin command: list current directory.
@param args List of args.  args[0] is "ls".
@return Always returns 1, to continue executing.
*/
int mmsh_ls(char **args)
{
	DIR *dp;
	struct dirent *ep;

	dp = opendir("./");
	if (dp != NULL) {
		while (ep = readdir(dp)) {
			printf("%s\n", ep->d_name);
		}
		closedir(dp);
	}
	else {
		perror("Couldn't open the directory");
	}
	return 1;
}

/**
@brief Bultin command: make directory.
@param args List of args.  args[0] is "mkdir".  args[1] is the directory.
@return Always returns 1, to continue executing.
*/
int mmsh_mkdir(char **args)
{
	if (args[1] == NULL) {
		fprintf(stderr, "mmsh: expected argument to \"mkdir\"\n");
	}
	else {
		if (mkdir(args[1], 0755) != 0) {
			perror("mmsh");
		}
	}
	return 1;
}


/**
@brief Builtin command: print help.
@param args List of args.  Not examined.
@return Always returns 1, to continue executing.
*/
int mmsh_help(char **args)
{
	int i;
	printf("Stephen Brennan's LSH\n");
	printf("Type program names and arguments, and hit enter.\n");
	printf("The following are built in:\n");

	for (i = 0; i < mmsh_num_builtins(); i++) {
		printf("  %s\n", builtin_str[i]);
	}

	printf("Use the man command for information on other programs.\n");
	return 1;
}

/**
@brief Builtin command: exit.
@param args List of args.  Not examined.
@return Always returns 0, to terminate execution.
*/
int mmsh_exit(char **args)
{
	return 0;
}

/**
@brief Launch a program and wait for it to terminate.
@param args Null terminated list of arguments (including program).
@return Always returns 1, to continue execution.
*/
int mmsh_launch(char **args)
{
	pid_t pid;
	int status;

	pid = fork();
	if (pid == 0) {
		// Child process
		if (execvp(args[0], args) == -1) {
			perror("mmsh");
		}
		exit(EXIT_FAILURE);
	}
	else if (pid < 0) {
		// Error forking
		perror("mmsh");
	}
	else {
		// Parent process
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 1;
}

/**
@brief Execute shell built-in or launch program.
@param args Null terminated list of arguments.
@return 1 if the shell should continue running, 0 if it should terminate
*/
int mmsh_execute(char **args)
{
	int i;

	if (args[0] == NULL) {
		// An empty command was entered.
		return 1;
	}

	for (i = 0; i < mmsh_num_builtins(); i++) {
		if (strcmp(args[0], builtin_str[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}

	return mmsh_launch(args);
}

#define LSH_RL_BUFSIZE 1024
/**
@brief Read a line of input from stdin.
@return The line from stdin.
*/
char *mmsh_read_line(void)
{
	int bufsize = LSH_RL_BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize);
	int c;

	if (!buffer) {
		fprintf(stderr, "mmsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		// Read a character
		c = getchar();

		if (c == EOF) {
			exit(EXIT_SUCCESS);
		}
		else if (c == '\n') {
			buffer[position] = '\0';
			return buffer;
		}
		else {
			buffer[position] = c;
		}
		position++;

		// If we have exceeded the buffer, reallocate.
		if (position >= bufsize) {
			bufsize += LSH_RL_BUFSIZE;
			buffer = realloc(buffer, bufsize);
			if (!buffer) {
				fprintf(stderr, "mmsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
@brief Split a line into tokens (very naively).
@param line The line.
@return Null-terminated array of tokens.
*/
char **mmsh_split_line(char *line)
{
	int bufsize = LSH_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token, **tokens_backup;

	if (!tokens) {
		fprintf(stderr, "mmsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, LSH_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += LSH_TOK_BUFSIZE;
			tokens_backup = tokens;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			if (!tokens) {
				free(tokens_backup);
				fprintf(stderr, "mmsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, LSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

/**
@brief Loop getting input and executing it.
*/
void mmsh_loop(void)
{
	char *line;
	char **args;
	int status;

	do {
		printf("> ");
		line = mmsh_read_line();
		args = mmsh_split_line(line);
		status = mmsh_execute(args);

		free(line);
		free(args);
	} while (status);
}

/**
@brief Main entry point.
@param argc Argument count.
@param argv Argument vector.
@return status code
*/
int main(int argc, char **argv)
{
	// Load config files, if any.

	// Run command loop.
	mmsh_loop();

	// Perform any shutdown/cleanup.

	return EXIT_SUCCESS;
}
