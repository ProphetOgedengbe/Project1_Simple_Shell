#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define MAX_COMMAND_LINE_LEN 1024
#define MAX_COMMAND_LINE_ARGS 128

char delimiters[] = " \t\r\n";
extern char **environ;

/* Global variable to track child process */
volatile pid_t current_child_pid = -1;
volatile bool timeout_occurred = false;

/* Signal handler for SIGINT (Ctrl-C) */
void sigint_handler(int sig) {
    printf("\n");
    fflush(stdout);
    
    if (current_child_pid > 0) {
        kill(current_child_pid, SIGINT);
    }
}

/* Signal handler for SIGALRM (timeout) */
void sigalrm_handler(int sig) {
    if (current_child_pid > 0) {
        printf("\nProcess exceeded 10 second limit. Terminating...\n");
        kill(current_child_pid, SIGKILL);
        timeout_occurred = true;
    }
}

/* Tokenize the command line and handle environment variables */
int tokenize(char *command_line, char **arguments) {
    int i = 0;
    char *token = strtok(command_line, delimiters);
    
    while (token != NULL && i < MAX_COMMAND_LINE_ARGS - 1) {
        if (token[0] == '$') {
            char *env_value = getenv(token + 1);
            if (env_value != NULL) {
                arguments[i] = env_value;
            } else {
                arguments[i] = "";
            }
        } else {
            arguments[i] = token;
        }
        i++;
        token = strtok(NULL, delimiters);
    }
    arguments[i] = NULL;
    return i;
}

/* Built-in command: cd */
void cmd_cd(char **arguments) {
    if (arguments[1] == NULL) {
        char *home = getenv("HOME");
        if (home != NULL) {
            chdir(home);
        }
    } else {
        if (chdir(arguments[1]) != 0) {
            perror("cd failed");
        }
    }
}

/* Built-in command: pwd */
void cmd_pwd() {
    char cwd[MAX_COMMAND_LINE_LEN];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd failed");
    }
}

/* Built-in command: echo */
void cmd_echo(char **arguments) {
    int i;
    for (i = 1; arguments[i] != NULL; i++) {
        printf("%s", arguments[i]);
        if (arguments[i + 1] != NULL) {
            printf(" ");
        }
    }
    printf("\n");
}

/* Built-in command: env */
void cmd_env(char **arguments) {
    int i;
    if (arguments[1] == NULL) {
        for (i = 0; environ[i] != NULL; i++) {
            printf("%s\n", environ[i]);
        }
    } else {
        char *value = getenv(arguments[1]);
        if (value != NULL) {
            printf("%s\n", value);
        }
    }
}

/* Built-in command: setenv */
void cmd_setenv(char **arguments) {
    if (arguments[1] != NULL) {
        char *equals = strchr(arguments[1], '=');
        if (equals != NULL) {
            *equals = '\0';
            char *name = arguments[1];
            char *value = equals + 1;
            setenv(name, value, 1);
        }
    }
}

/* Execute command with pipe */
void execute_pipe(char **arguments) {
    int pipefd[2];
    pid_t pid1, pid2;
    int i;
    char **cmd2;
    
    /* Find the pipe symbol */
    for (i = 0; arguments[i] != NULL; i++) {
        if (strcmp(arguments[i], "|") == 0) {
            arguments[i] = NULL;
            cmd2 = &arguments[i + 1];
            break;
        }
    }
    
    if (pipe(pipefd) < 0) {
        perror("pipe failed");
        return;
    }
    
    /* First command */
    pid1 = fork();
    if (pid1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp(arguments[0], arguments);
        perror("execvp() failed");
        exit(1);
    }
    
    /* Second command */
    pid2 = fork();
    if (pid2 == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execvp(cmd2[0], cmd2);
        perror("execvp() failed");
        exit(1);
    }
    
    /* Parent closes pipes and waits */
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

/* Check for I/O redirection */
bool handle_redirection(char **arguments) {
    int i;
    int fd;
    
    for (i = 0; arguments[i] != NULL; i++) {
        /* Output redirection */
        if (strcmp(arguments[i], ">") == 0) {
            arguments[i] = NULL;
            fd = open(arguments[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            return true;
        }
        /* Input redirection */
        else if (strcmp(arguments[i], "<") == 0) {
            arguments[i] = NULL;
            fd = open(arguments[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            return true;
        }
    }
    return false;
}

/* Check if command contains a pipe */
bool has_pipe(char **arguments) {
    int i;
    for (i = 0; arguments[i] != NULL; i++) {
        if (strcmp(arguments[i], "|") == 0) {
            return true;
        }
    }
    return false;
}

int main() {
    char command_line[MAX_COMMAND_LINE_LEN];
    char *arguments[MAX_COMMAND_LINE_ARGS];
    char cwd[MAX_COMMAND_LINE_LEN];
    int arg_count;
    bool background;
    pid_t pid;
    int status;
    
    /* Set up signal handlers */
    signal(SIGINT, sigint_handler);
    signal(SIGALRM, sigalrm_handler);
    
    while (true) {
        /* Get current working directory for prompt */
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s> ", cwd);
        } else {
            printf("> ");
        }
        fflush(stdout);
        
        /* Read input */
        if ((fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) && ferror(stdin)) {
            fprintf(stderr, "fgets error\n");
            exit(0);
        }
        
        /* Check for EOF */
        if (feof(stdin)) {
            printf("\n");
            fflush(stdout);
            fflush(stderr);
            return 0;
        }
        
        /* Remove newline */
        command_line[strlen(command_line) - 1] = '\0';
        
        /* Skip empty lines */
        if (command_line[0] == '\0') {
            continue;
        }
        
        /* Tokenize */
        arg_count = tokenize(command_line, arguments);
        
        if (arg_count == 0) {
            continue;
        }
        
        /* Check for background process */
        background = false;
        if (strcmp(arguments[arg_count - 1], "&") == 0) {
            background = true;
            arguments[arg_count - 1] = NULL;
            arg_count--;
        }
        
        /* Handle built-in commands */
        if (strcmp(arguments[0], "exit") == 0) {
            break;
        } else if (strcmp(arguments[0], "cd") == 0) {
            cmd_cd(arguments);
        } else if (strcmp(arguments[0], "pwd") == 0) {
            cmd_pwd();
        } else if (strcmp(arguments[0], "echo") == 0) {
            cmd_echo(arguments);
        } else if (strcmp(arguments[0], "env") == 0) {
            cmd_env(arguments);
        } else if (strcmp(arguments[0], "setenv") == 0) {
            cmd_setenv(arguments);
        } else if (has_pipe(arguments)) {
            /* Handle piped commands in parent (no background support for pipes) */
            execute_pipe(arguments);
        } else {
            /* External command - fork and execute */
            pid = fork();
            
            if (pid < 0) {
                perror("fork failed");
            } else if (pid == 0) {
                /* Child process */
                signal(SIGINT, SIG_DFL);
                handle_redirection(arguments);
                execvp(arguments[0], arguments);
                perror("execvp() failed");
                exit(1);
            } else {
                /* Parent process */
                current_child_pid = pid;
                timeout_occurred = false;
                
                if (!background) {
                    /* Set up 10 second timer for foreground processes */
                    alarm(10);
                    
                    waitpid(pid, &status, 0);
                    
                    /* Cancel alarm */
                    alarm(0);
                    
                    current_child_pid = -1;
                } else {
                    current_child_pid = -1;
                }
            }
        }
    }
    
    return 0;
}