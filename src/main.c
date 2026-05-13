/**************************************************************************
 * @Author: Denver Cowan
 * @Date: 4/27/2026
 * TSH - Tiny Shell
 *************************************************************************/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @brief This function reads a line from input into a buffer
 * @details This function uses getchar() to read the input line into a buffer
 * until it reaches EOF, later this line will be parsed by another function.
 * @return a char* pointer to the buffer to be passed into the parsing function.
 */

#define TSH_RL_BUFSIZE 1024
char *tsh_read_line(void) {
  int bufsize = TSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "tsh: allocation error for buffer.\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
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

/**
 * @brief
 * @details
 * @param
 * @return
 */

#define TSH_TOK_BUFSIZE 64
#define TSH_TOK_DELIM " \t\r\n\a"
char *outfile = NULL;
char *infile = NULL;

char **tsh_split_line(char *line) {
  int bufsize = 64;
  int position = 0;
  char **tokens = malloc(bufsize * sizeof(char *)); // |ptr | ptr
  char *token;                                      //    |     |
                                                    //  "ls"   "-l"
  //  this is a simple view of how it would be laid out in memory.
  if (!tokens) {
    fprintf(stderr, "tsh: token allocation error\n");
    exit(EXIT_FAILURE);
  }

  outfile = NULL;
  infile = NULL;

  token = strtok(line, TSH_TOK_DELIM);
  while (token != NULL) {
    if (strcmp(token, ">") == 0) {
      token = strtok(NULL, TSH_TOK_DELIM);

      if (token == NULL) {
        fprintf(stderr, "tsh: expected filename after >\n");
        tokens[position] = NULL;
        return tokens;
      }
      outfile = token;
      token = strtok(NULL, TSH_TOK_DELIM);
      continue;
    }

    if (strcmp(token, "<") == 0) {
      token = strtok(NULL, TSH_TOK_DELIM);

      if (token == NULL) {
        fprintf(stderr, "tsh: expected filename after <\n");
        tokens[position] = NULL;
        return tokens;
      }
      infile = token;
      token = strtok(NULL, TSH_TOK_DELIM);
      continue;
    }

    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += TSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char *));
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

/*
 * @brief this is the launch function for non-piped commands
 */
int tsh_launch(char **args, char *outfile, char *infile) {
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // child process
    if (outfile != NULL) {
      int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

      if (fd < 0) {
        perror("tsh: open");
        _exit(EXIT_FAILURE);
      }

      if (dup2(fd, STDOUT_FILENO) < 0) { // STDOUT_FILENO == 1
        perror("tsh: dup2");
        close(fd);
        _exit(EXIT_FAILURE);
      }
      close(fd);
    }

    if (infile != NULL) {
      int fd = open(infile, O_RDONLY);

      if (fd < 0) {
        perror("tsh: open");
        _exit(EXIT_FAILURE);
      }

      if (dup2(fd, STDIN_FILENO) < 0) { // STDIN_FILENO == 0
        perror("tsh: dup2");
        close(fd);
        _exit(EXIT_FAILURE);
      }
      close(fd);
    }

    if (execvp(args[0], args) == -1) {
      perror("tsh: error with execvp");
      _exit(EXIT_FAILURE);
    }

  } else if (pid < 0) {
    // error forking
    perror("tsh: error forking");
  } else {
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/* function declaration for find_pipe utility */
int find_pipe(char **args);

/*
 * @ brief this is the launch funciton for piped commands
 */
int tsh_launch_pipe(char **args) {
  int pipe_pos = find_pipe(args);
  if (pipe_pos == -1) {
    fprintf(stderr, "TSH: no pipe found\n");
    return 1;
  }

  args[pipe_pos] = NULL;

  // split the args into two arrays
  char **left_args = args;
  char **right_args = &args[pipe_pos + 1];

  if (left_args[0] == NULL || right_args[0] == NULL) {
    fprintf(stderr, "TSH: invalid pipe\n");
    return 1;
  }

  int fd[2];

  // create pipe
  if (pipe(fd) == -1) {
    perror("TSH: pipe");
    return 1;
  }

  pid_t left_pid = fork();
  if (left_pid < 0) {
    perror("tsh: left child fork");
    close(fd[0]);
    close(fd[1]);
    return 1;
  }

  // left child
  if (left_pid == 0) {

    // stdout -> pipe write end
    if (dup2(fd[1], STDOUT_FILENO) == -1) {
      perror("tsh: left dup2");
      _exit(EXIT_FAILURE);
    }

    // close unused fds so process doesn't hang
    close(fd[0]);
    close(fd[1]);

    execvp(left_args[0], left_args);

    perror("tsh: pipe left child");
    _exit(EXIT_FAILURE);
  }

  pid_t right_pid = fork();
  if (right_pid < 0) {
    perror("tsh: right child fork\n");
    close(fd[0]);
    close(fd[1]);
    waitpid(left_pid, NULL, 0);
    return 1;
  }

  // right child
  if (right_pid == 0) {

    // stdin <- pipe read end
    if (dup2(fd[0], STDIN_FILENO) == -1) {
      perror("TSH: right dup2");
      _exit(EXIT_FAILURE);
    }

    // close unused fds
    close(fd[1]);
    close(fd[0]);

    execvp(right_args[0], right_args);

    perror("tsh: right child pipe");
    _exit(EXIT_FAILURE);
  }

  // parent no longer needs pipe fds
  close(fd[0]);
  close(fd[1]);

  // wait for both children
  waitpid(left_pid, NULL, 0);
  waitpid(right_pid, NULL, 0);

  return 1;
}

/* Function declarations for shell builtins */
int tsh_cd(char **args);
int tsh_help(char **args);
int tsh_exit(char **args);

/* list of builtin commands */
char *builtin_str[] = {"cd", "help", "exit"};

int (*builtin_func[])(char **) = {&tsh_cd, &tsh_help, &tsh_exit};

int tsh_num_builtins() { return sizeof(builtin_str) / sizeof(char *); }

/* Builtin function implementations */

int tsh_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "tsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("tsh");
    }
  }
  return 1;
}

int tsh_help(char **args) {
  int i;
  printf("Denver Cowan's TSH (Tiny Shell)\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in: \n");

  for (i = 0; i < tsh_num_builtins(); i++) {
    printf(" %s\n", builtin_str[i]);
  }

  printf("Use the man command for information of other programs.\n");
  return 1;
}

int tsh_exit(char **args) { return 0; }

int tsh_execute(char **args) {
  int i;

  if (args[0] == NULL) {
    // empty command was entered
    return 1;
  }

  // added for running piped commands
  int pipe_pos = find_pipe(args);
  if (pipe_pos != -1) {
    return tsh_launch_pipe(args);
  }

  for (i = 0; i < tsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return tsh_launch(args, outfile, infile);
}

/*
 * @brief this method determines if the input string has a pipe operator
 * and returns it's position, or -1 if it does not have one.
 */

int find_pipe(char **args) {
  for (int i = 0; args[i] != NULL; i++) {
    if (strcmp(args[i], "|") == 0) {
      return i;
    }
  }
  return -1;
}

/*
 * @brief This is the core loop of the shell
 * */
void tsh_loop(void) {
  char *line;
  char **args;
  int status;

  do {
    printf(">>>> ");
    line = tsh_read_line();
    args = tsh_split_line(line);
    status = tsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

int main(int argc, char **argv) {
  tsh_loop();
  return EXIT_SUCCESS;
}
