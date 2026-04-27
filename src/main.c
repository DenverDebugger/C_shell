/**************************************************************************
 * @Author: Denver Cowan
 * @Date: 4/27/2026
 * TSH - Tiny Shell
 *************************************************************************/


/* This is the core loop of the shell */
do {
	printf(">>>> ");
	line = tsh_read_line();
	args = tsh_split_line(line);
	status = tsh_execute(args);

	free(line);
	free(args);
} while (status);

#define TSH_RL_BUFSIZE 1024
char *tsh_read_line(void)
{
	int bufsize = TSH_RL_BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize);
	int c;

	if (!buffer) {
		fprintf(stderr, "tsh: allocation error for buffer.\n");
		exit(EXIT_FAILURE);
	}

	while(1) {
		c = getchar();

		if (c == EOF || c == '\n') {
			buffer[position] = '\0';
			return buffer;
		} else {
			buffer[position] = c;
		}
		position++;

		if (position >= bufsize) {
			bufsize += TSH_RL_BUFSIZE;
			buffer = realloc(buffer, bufsize);
			if (!buffer) {
				fprintf(stderr, "tsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

#define TSH_TOK_BUFSIZE 64
#define TSH_TOK_DELIM " \t\r\n\a"
char **tsh_split_line(char *line)
{
	int bufsize = 64;
	int position = 0;
	char **tokens = malloc(bufsize * sizeof(char*)); // |ptr | ptr
	char *token;					//    |     |
							//  "ls"   "-l" 
							//  this is a simple view of how it would be laid out in memory. 
	if (!tokens) {
		fprintf(stderr, "tsh: token allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, TSH_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += TSH_TOK_BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			if (!tokens) {
				fprintf(stderr, "tsh: token allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL, TSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

int tsh_launch(char **args)
{
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid == 0) {
		// child process
		if (execvp(args[0], args) == -1) {
			perror("tsh: error with execvp");
		}
		exit(EXIT_FAILURE);
	} else if { (pid < 0) 
		//error forking
		perror("tsh: error forking");
	} else {
		do { 
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 1;
}























































































