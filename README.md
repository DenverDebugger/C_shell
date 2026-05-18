# C_shell
This is a tiny shell written in C for learning purposes. 

I took Operating Systems and Computer Architecture in College.... but college was busy, and didn't allow me to really dive as deeply into systems programming as I wanted to.

This shell is largely based on the one built here: https://brennan.io/2015/01/16/write-a-shell-in-c/#fnref:1
- I think Stephen does an excellent job explaining the process conceptually, and the code quality is good. 

However, just retyping his tutorial is not goint to be useful for learning, you should really dig into the what the code, and specifically the system calls are doing. Stephen links to the POSIX site, but I personally find this site to be a bit more readable: https://man7.org/linux/man-pages/index.html 
- I would recommend using the site to search for and read about the system calls being used. (`fork, execvp, malloc, realloc, dup2, strtok, strcmp` )

start with implementing his version, then come back to this when you want to implement piping and redirection. I'm not going to cover the rest since He has already done a great job.

# ===== Tutorial Section =====
## Redirection
So, let's start with implementing redirection. The code below is basically just the modifications needed to tsh_split_line() to support redirection, the rest of the function stays the same. 
```
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
    ---- remains the same as before. 
```

So, as you can see, we’ll need to declare a few new variables (`infile` and `outfile`). These are used to feed file content into programs or redirect program output into files.

Our loop still runs until `token == NULL`; however, now we also check whether we encounter a `<` or `>` token in the command line. If we do, we call `strtok()` again so that `token` now contains the filename that follows the redirection operator.

If that token is `NULL` (meaning no filename was specified after `>` or `<`), we throw an error. Otherwise, we store the token as either `outfile` or `infile`.

After storing the filename, we call `strtok()` again to advance to the next token in the command line and continue parsing. This is important because, in a compound command like:

    grep foo < in.txt > out.txt

we do not want `in.txt` or `out.txt` to be treated as normal command arguments. Instead, they are consumed as part of the redirection operation and skipped over during normal argument parsing.

The same process is repeated for the `<` operator, except this time we store the filename in `infile`.

Once that is done, the rest of the method processes the remaining command-line arguments as usual. 

## Piping
I decided to implement piping in it's own `tsh_launch_pipe()` function and also use a helper called `find_pipe()`. This is just to keep the functions from getting too convoluted. We'll start with the `find_pipe()` helper since it is short and easy to reason about. 
```
int find_pipe(char **args) {
  for (int i = 0; args[i] != NULL; i++) {
    if (strcmp(args[i], "|") == 0) {
      return i;
    }
  }
  return -1;
}
```
We're just taking in the argument string and checking if we encounter a pipe `|` operator and returning it's position, otherwise we just return the default not found value -1.

Now that we have that in place we will add some logic to our `tsh_execute()` function to determine when we need to branch our execution to the soon to be implemented `tsh_launch_pipe()` function. 
```
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
  ...remains the same. 
```
This function remains the same as before, just with the addition of checking if we have a pipe and calling `tsh_launch_pipe()` if we do. 

Now for the tsh_launch_pipe() function, it's a lot of code but basically we check there is a pipe present with out `find_pipe()` helper, then we insert `NULL` in the place of the `|` operator, then we split up the left and right side of the argument string into `left_args` & `right_args`, check for errors, then we create a file descriptor table `fd` to use as our pipe by calling `pipe()`, then we fork the left arg process and wire it's `STDOUT` to the write end of the pipe and execute it with `execvp`, we do the same thing for the right arg process but wire it's `STDIN` to the pipes read end, then in the outer parent scope we close up the file descriptors and wait on the forked processes to finish. 
```
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
```


Resources:
1. The C Programming Language by Kernighan and Ritchie (K&R)
    - I think appendix B will be the most useful.
    - This book is a piece of programmer lore though, so it wouldn't hurt to read/skim the whole thing.
2. https://www.learn-c.org/en/Welcome
    - This is a more interactive/concise version of what K&R provides.
3. AI Tools
    - I think if there are any gaps the above don't satisfy then using your favorite LLM or agentic tool is going to be your best bet.
