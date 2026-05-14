# C_shell
This is a tiny shell written in C for learning purposes. 

I took Operating Systems and Computer Architecture in College.... but college was busy, and didn't allow me to really dive as deeply into systems programming as I wanted to.

This shell is largely based on the one built here: https://brennan.io/2015/01/16/write-a-shell-in-c/#fnref:1
- I think Stephen does an excellent job explaining the process conceptually, and the code quality is good. 

However, just retyping his tutorial is not goint to be useful for learning, you should really dig into the what the code, and specifically the system calls are doing. Stephen links to the POSIX site, but I personally find this site to be a bit more readable: https://man7.org/linux/man-pages/index.html 
- I would recommend using the site to search for and read about the system calls being used. (chdir, fork, exec, malloc, etc. )

start with implementing his version, then come back to this when you want to implement piping and redirection. I'm not going to cover the rest since He has already done a great job.

# ========== Tutorial Section ==========
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

Resources:
1. The C Programming Language by Kernighan and Ritchie (K&R)
    - I think appendix B will be the most useful.
    - This book is a piece of programmer lore though, so it wouldn't hurt to read/skim the whole thing.
2. https://www.learn-c.org/en/Welcome
    - This is a more interactive/concise version of what K&R provides.
3. AI
    - I think if there are any gaps the above don't satisfy then using your favorite LLM or agentic tool is going to be your best bet, unless you happen to personally know someone that can answer your questions.
