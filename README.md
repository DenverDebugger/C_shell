# C_shell
This is a tiny shell written in C for learning purposes. 

I took Operating Systems and Computer Architecture in College.... but college was busy, and didn't allow me to really dive as deeply into systems programming as I wanted to.

This shell is largely based on the one built here: https://brennan.io/2015/01/16/write-a-shell-in-c/#fnref:1
    - I think Stephen does an excellent job explaining the process conceptually, and the code quality is good. 

However, just retyping his tutorial is not goint to be useful for learning, you should really dig into the what the code, and specifically the system calls are doing. Stephen links to the POSIX site, but I personally find this site to be a bit more readable: https://man7.org/linux/man-pages/index.html 
    - I would recommend at a minimum using the site to search for and read about the system calls being used. (chdir, fork, exec, malloc, etc. )

I would recommend implementing his version, then coming back to this when you want to implement piping and redirection. I'm not going to cover the rest since He has already done a great job.

Resources:
1. The C Programming Language by Kernighan and Ritchie (K&R)
    - I think appendix B will be the most useful.
    - This book is a piece of programmer lore though, so it wouldn't hurt to read/skim the whole thing.
2. https://www.learn-c.org/en/Welcome
    - This is a more interactive/concise version of what K&R provides.
3. AI
    - I think if there are any gaps the above don't satisfy then using your favorite LLM or agentic tool is going to be your best bet, unless you happen to personally know someone that can answer your questions.
