# Operating Systems and Networks Assignment 3
## `CASH`: Cliché Average SHell

### Build and Run:
The file already contains the precompiled files and you can launch the shell just by running `./cash`.
If however, you wish to recompile the shell, run the following commands
```bash
make clean
make
./cash
```
`make clean` removes all the object files and the compiled executable.

The `make` command will compile all the `.c` files inside `./src` into `.o` files. It then makes the `obj` directory if it doesn't exist and moves all the `.o` files into that directory. Finally, it links all the object files and the `./cash` executable is generated.

### Specification Implementation Details

#### Specification 1: Input/Output Redirection

The `execCommand()` method in `utilities.c` has been modified to parse the command and check if any kind of I/O redirection is required. If it is, then the required file is opened and the required streams are connected to them. Then the functions corresponding to the command is executed. After returning from the command specific function, at the end of the `execCommand()` function, the files are closed and streams are set to default.

#### Specification 2: Command Pipelines

A new function has been added in the call stack, called `handlePipes()`, which is called by `cleanCommand()` and inturn calls `execCommand()` to execute it acfter handling piping logic.

Inside `handlePipes()`, we first calculate the number of pipes. If it is 0, we directly send the input string to `execCommand()`. Instead, if there are some pipes, we split to commands. Say there are `n` commands, then we create `n` pipes, meaning `2n` file descriptors and populate them using the `pipe()` function. We then create `n` child processes of the shell. We link the `STDOUT` of the `ith` child to the `ith` write end of the pipe and the `STDIN` of the `ith` child to the `(i-1)th` read end of the pipe. So, the `STDIN` of the first child is still default and the `STDOUT` of the last child is also still default. This creates a pipe chain in which the `ith` child reads the input from the output of the `(i-1)th` child.

#### Specification 3:I/O Redirection within Command Pipelines

The way the first two specifications have been implemented, this specification has inherently been implemented.

#### Specification 4: User-defined Commands

1. `setenv var [value]`: Implemented in `cash_setenv()` in `functions.c`. The command usage is checked and then the required environment variable is set using `setenv()`.

2. `unsetenv var`: Implemented in `cash_unsetenv()` in `functions.c`. The command usage is checked and then the required environment variable is unset using `unsetenv()`.

3. `jobs`: Implemented in `jobs()` in `functions.c`. Iterates over the children in child pool and fetches each one's status from the processes' `proc/<pid>/stat` file.

4. `kjob <job number> <signal number>`: Implemented in `kjob()` in `functions.c`. Checks usage, then verifies the given job number and finally, extracts the pid of the selected job from the job pool and then sent the required signal using the `kill()` syscall.

5. `fg <job number>`: Implemented in `fg()` in `functions.c`. Checks command usage, then verifies the given job number. Once it is confirmed that the job is a valid one, it gets its pid from the child pool, gets its process group, removes the child from the pool, tells the shell to ignore all `STDIN` and `STDOUT` related events and then gives terminal control to the process group belonging to said job. The job is also sent the `SIGCONT` signal, incase it had stopped in the background. While the process runs in the foreground, the shell waits on it using the `wait()` syscall. Once the foreground process terminates or has been stopped, the terminal control is returned to the shell and the default response to `STDIN` and `STDOUT` events is restored.

6. `bg <job number>`: Implemented in `bg()` in `functions.c`. Checks command usage, then verifies the given job number. Once it is confirmed that the job is a valid one, it gets its pid from the child pool. Then it sends the `SIGCONT` signal to that process using the `kill()` method, to tell it to change its status from stopped to running.

7. `overkill`: Implemented in `overkill()` in `functions.c`. It iterates over the children in the pool and sends each one the `SIGKILL` signal and then removes them from the pool.

8. `quit`: Simply calls `exit(0)`. As for exit on `Ctrl+D`, the shell uses the `fgets()` method to read input. `fgets()` returns a `NULL` pointer when it detects the `EOF` character. A check is done on the return value of `fgets()`. If it happens to be null, the infinite I/O loop is broken out of and the shell exits.

#### Specification 5: Signal Handling

For both the signals, a new action was defined using the `sigaction` structure.

1. `Ctrl-Z`: The hanlder for this is relatively straightforward. It makes a new line and prints the prompt string again. To make sure that when a Ctrl-Z is hit on a foreground process, it is added to the job pool, one bit of modification needed to be made in `fExec()` and `fg()`. The `wait()` was replaced with `waitpid()` and the `WUNTRACED` flag was set. This way, we could inspect the status code to check if the process had been stopped or terminated using the `WIFSTOPPED` macro. If the process has been stopped, it is added to the child pool, because it has been sent to the background.

2. `Ctrl-C`: The hanlder for this is essentially the same as that for the previous one. It prints a new line and prints the prompt string again.

### Bonus Implementation

#### Bonus Specification 1: Last Working Directory

A global variable called `PREV_LOC[]` is used to store the previous working directory of the shell. Whenever `cd` is run, it is check if the given directory is valid. If so, `PREV_LOC[]` is changed to the current directory and then the `chdir()` syscall is performed. Otherwise, `PREV_LOC[]` is not altered. If the argument passed to `cd` is `-`, then `PREV_LOC[]` is passes as the argument to `chdir()`.

#### Bonus Specification 2: Exit Codes

A global varibale called `exitCode` is defined. It is initialized to 0 on the initial call to `generatePS()` to indicate that no command has been run yet. If `exitCode` is 0, no face is printed. At the start of `execCommand()`, `exitCode` is set to 1, which indicates a successful execution. Now, at every point that is called from `execCommand()`, wherever there is some error checking for potential problems, if the problem has occured, `exitCode` has been set to -1 to indicate failure. So, by the time `execCommand()` has been popped off the stack, if `exitCode` is still 1, a happy face is printed on the next prompt string, otherwise a sad face is printed.

### Custom Commands

One custom command was added to the shell as well, which wasn't mentioned in the requirements:

* `env` → Lists all the enivroment variables in the current instance of the shell. Implemented in `cash_env()` in `functions.c`.