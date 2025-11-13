Quash – A Simple Unix Shell

Author: Prophet O.

Course: Intro to C / Systems Programming

Project: Quash (Project #1)

Description: A basic command-line shell that replicates core behaviors of sh/bash using C, process forking, and Unix system calls.

⸻

1. Overview

Quash (QUick bash) is a simplified command-line shell written in C.
The goal of this project is to understand:
	•	How command-line interfaces work
	•	How user input is parsed into commands
	•	How processes are created and managed (fork, execvp, waitpid)
	•	How signals affect running processes
	•	How to implement built-in shell behavior (e.g., cd, exit)

Quash supports running external commands, built-in commands, background jobs, and basic directory management.
This project builds foundational knowledge required for operating systems, process schedulers, and more advanced shell features.

⸻

2. Features Implemented

✔️ User Input Loop

Quash runs an infinite loop that:
	1.	Prints a prompt: quash> 
	2.	Reads a full command line
	3.	Tokenizes input into an argument array (argv)
	4.	Executes built-ins or external programs depending on the command

✔️ Built-In Commands

Command	Description
cd <directory>	Changes the working directory
pwd	Prints the current working directory
exit / quit	Terminates the shell

These commands are handled inside the shell because they modify shell state and cannot be delegated to a child process.

✔️ Running External Programs

Commands such as:

ls
cat file.txt
ps -aux

run using:
	•	fork() to create a child
	•	execvp() to launch the program
	•	waitpid() to wait for foreground processes

If the command is not built-in, Quash assumes it is an external program and attempts to execute it.

✔️ Background Execution (&)

If a user appends:

command &

The shell:
	•	Removes the & from the argument list
	•	Launches the command without waiting for it
	•	Prints the background process PID

This demonstrates asynchronous process creation.

⸻

3. System Architecture & Design Choices

3.1 Main Components
	1.	Input Handling — Uses fgets() to safely read input.
	2.	Command Parser — Splits input into tokens using strtok().
	3.	Built-In Command Dispatcher — Checks whether the first token matches a built-in.
	4.	Process Executor — Handles fork, execvp, and waitpid.
	5.	Background Job Handling — Checks for & and skips waiting.

3.2 Data Structures
	•	char line[] — Stores raw user input.
	•	char* argv[] — Pointer array holding parsed command arguments.
	•	int background — Flag indicating background execution.
	•	Fixed-size arrays were used to keep implementation simple and predictable.

3.3 Error Handling
	•	Invalid paths in cd → prints error with perror.
	•	Unknown commands → execvp prints "command not found" style error.
	•	Empty input lines are safely ignored.
	•	If fork fails, the shell prints an error and continues.

⸻

4. Process Creation Model

To execute external commands, Quash uses the standard Unix shell model:
	1.	Parent calls fork()
	•	Child gets a copy of the parent’s memory space.
	2.	Child calls execvp()
	•	Replaces itself with the requested program.
	3.	Parent calls waitpid()
Unless the process is running in the background.

This model mirrors the architecture of real shells like Bash.

⸻

5. Signal Behavior

Foreground Processes

The parent waits using:

waitpid(pid, &status, 0);

Background Processes

The parent does not wait:

waitpid(pid, &status, WNOHANG);

Signal Defaults
	•	Ctrl+C (SIGINT) terminates the running child process, not the shell.
	•	The shell itself ignores some signals to avoid exiting unintentionally.

⸻

6. Testing & Example Commands

Examples of commands supported in Quash:

quash> ls
quash> pwd
quash> cd /usr/bin
quash> echo Hello World
quash> sleep 5 &
quash> exit

All standard Unix commands available on Codio work with this shell.

⸻

7. Limitations & Possible Future Enhancements

This assignment intentionally keeps Quash simple. Possible expansions:

⭐ Not implemented (but good future enhancements)
	•	I/O redirection:
	•	command > file
	•	command < file
	•	Pipelines:
	•	ls | grep txt
	•	Job control:
	•	jobs, fg, bg
	•	Command history
	•	Tab completion
	•	Persistent environment variables

These would require additional data structures, inter-process communication, and possibly sigaction().

⸻

8. How to Run

Compile

gcc quash.c -o quash

Run

./quash

Works on:
	•	Codio Linux environment
	•	WSL / Linux / macOS
	•	Any Unix-like system with GCC

⸻

9. Repository Notes
	•	All source code is authored by: Prophet O.
	•	README doubles as the project report.
	•	Project is private per assignment instructions.
	•	Instructor GitHub access provided.

⸻

10. Conclusion

This project provides hands-on experience with:
	•	Parsing user input
	•	Managing processes
	•	Understanding how shells execute commands
	•	Using fork, execvp, waitpid, and basic signals

Quash demonstrates the core ideas behind real shells like Bash while remaining small enough to understand completely. This project was extremely helpful in strengthening my understanding of operating system internals and Unix process management.

