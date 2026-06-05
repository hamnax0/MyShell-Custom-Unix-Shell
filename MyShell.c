#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#define MAX_INPUT_LEN   1024
#define MAX_ARGS          64
#define MAX_HISTORY       10
#define PROMPT_BUFSIZE   600

#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define CYAN    "\033[1;36m"
#define RESET   "\033[0m"

int shell_running=1;

char history[MAX_HISTORY][MAX_INPUT_LEN];
int  history_count=0;

/* ── Banner & Prompt ─────────────────────────────────────── */

void show_banner() 
{
    char *username=getenv("USER");
    if (!username) 
		username="user";

    printf(CYAN "********************************************************************************\n" RESET);
    printf(CYAN "                                                                                \n");
    printf("                             ___ _        _ _ \n");
    printf("                   _ __ _  _/ __| |_  ___| | |\n");
    printf("                  | '  \\ || \\__ \\ ' \\/ -_) | |\n");
    printf("                  |_|_|_\\_, |___/_||_\\___|_|_|\n");
    printf("                        |__/                  \n");
    printf(RESET);
    printf(CYAN "********************************************************************************\n" RESET);

    int welcome_len=26+strlen(username);
    int padding=(80 - welcome_len) / 2;
    if (padding<0) 
		padding=0;

    printf(GREEN);
    for(int i=0;i<padding;i++) 
		printf(" ");
		
    printf("Welcome, %s! Type 'help' for commands.\n", username);
    printf(RESET);

    printf(CYAN "********************************************************************************\n\n" RESET);
}

char *get_prompt() 
{
    static char prompt_buf[PROMPT_BUFSIZE];

    time_t now=time(NULL);
    struct tm *tm_now=localtime(&now);

    char time_str[16];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_now);

    char cwd[512];
    if (getcwd(cwd,sizeof(cwd)) == NULL)
        strcpy(cwd, "~");

    snprintf(prompt_buf, PROMPT_BUFSIZE, "%s:%s", time_str, cwd);
    return prompt_buf;
}

/* ── History ─────────────────────────────────────────────── */

void save_to_history(char *cmd) 
{
    if (strlen(cmd)==0 || strcmp(cmd, "!!")==0) 
		return;

    if (history_count == MAX_HISTORY) 
	{
        for (int i=0; i<MAX_HISTORY-1; i++)
            strcpy(history[i], history[i+1]);
        
		history_count = MAX_HISTORY - 1;
    }

    strcpy(history[history_count], cmd);
    history_count++;
}

void show_history() 
{
    if (history_count == 0) 
	{
        printf(YELLOW "  No commands in history yet.\n" RESET);
        return;
    }
    
    printf(CYAN "  ── Command History ──\n" RESET);
    for (int i = 0; i < history_count; i++)
        printf(GREEN "  [%d] " RESET "%s\n", i+1, history[i]);
}

char *get_last_command() 
{
    if (history_count==0) 
		return NULL;
		
    return history[history_count-1];
}

/* ── Input Parsing ───────────────────────────────────────── */

void strip_newline(char *line) 
{
    int len=strlen(line);
    if (len > 0 && line[len - 1] =='\n')
        line[len-1] = '\0';
}

void parse_args(char *input, char **args, int *run_in_background) 
{
    for (int i = 0; i < MAX_ARGS; i++) 
		args[i] = NULL;
		
    *run_in_background = 0;

    int len = strlen(input);
    if (len > 0 && input[len - 1] == '&') 
	{
        *run_in_background = 1;
         input[len - 1]= '\0';
    }

    int i = 0;
    char *tok = strtok(input, " ");
    while (tok != NULL && i < MAX_ARGS - 1) 
	{
        args[i++] = tok;
        tok       = strtok(NULL, " ");
    }
    
    args[i] = NULL;
}

/* ── Built-in Commands ───────────────────────────────────── */

int cmd_cd(char **args) 
{
    if (args[1] == NULL) 
	{
        char *home_dir = getenv("HOME");
        if (home_dir && chdir(home_dir) != 0)
            perror(RED "  cd error" RESET);
    }
	else 
	{
        if (chdir(args[1]) != 0)
            printf(RED "  cd: No such directory: %s\n" RESET, args[1]);
    }
    return 1;
}

int cmd_help(char **args) 
{
    printf(CYAN "\n  ╔══════════════════════════════════════════════════╗\n");
    printf(     "  ║              MYSHELL — HELP MENU                ║\n");
    printf(     "  ╠══════════════════════════════════════════════════╣\n" RESET);

    printf(GREEN "  %-20s" RESET " %s\n", "cd <dir>",    "Change directory");
    printf(GREEN "  %-20s" RESET " %s\n", "cd",          "Go to home directory");
    printf(GREEN "  %-20s" RESET " %s\n", "history",     "Show command history");
    printf(GREEN "  %-20s" RESET " %s\n", "!!",          "Repeat last command");
    printf(GREEN "  %-20s" RESET " %s\n", "cmd > file",  "Redirect output to file");
    printf(GREEN "  %-20s" RESET " %s\n", "cmd < file",  "Redirect input from file");
    printf(GREEN "  %-20s" RESET " %s\n", "cmd >> file", "Append output to file");
    printf(GREEN "  %-20s" RESET " %s\n", "cmd1 | cmd2", "Pipe output to next command");
    printf(GREEN "  %-20s" RESET " %s\n", "cmd &",       "Run command in background");
    printf(GREEN "  %-20s" RESET " %s\n", "exit / quit", "Exit the shell");

    printf(CYAN "  ╚══════════════════════════════════════════════════╝\n\n" RESET);
    return 1;
}

int cmd_exit(char **args) 
{
    printf(YELLOW "\n  Goodbye! Exiting MyShell...\n\n" RESET);
    shell_running = 0;
    return 0;
}

/* ── Redirection ─────────────────────────────────────────── */

void setup_redirection(char **args) 
{
    for (int i = 0; args[i] != NULL; i++) 
	{

        if (strcmp(args[i], ">") == 0) 
		{
            if (args[i + 1] == NULL) 
			{
                printf(RED "  Error: Missing filename after >\n" RESET);
                exit(EXIT_FAILURE);
            }
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            
			if (fd == -1) 
			{ 
				perror(RED "  open()" RESET); exit(EXIT_FAILURE); 
			}
            
			dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = args[i + 1] = NULL;
            return;

        } 
		else if (strcmp(args[i], ">>") == 0) 
		{
            if (args[i + 1] == NULL) 
			{
                printf(RED "  Error: Missing filename after >>\n" RESET);
                exit(EXIT_FAILURE);
            }
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            
			if (fd == -1) 
			{ 
				perror(RED "  open()" RESET); exit(EXIT_FAILURE); 
			}
			
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = args[i + 1] = NULL;
            return;

        } 
		else if (strcmp(args[i], "<") == 0) 
		{
            if (args[i + 1] == NULL) 
			{
                printf(RED "  Error: Missing filename after <\n" RESET);
                exit(EXIT_FAILURE);
            }
            
            int fd = open(args[i + 1], O_RDONLY);
            
			if (fd == -1) 
			{ 
				perror(RED "  open()" RESET); exit(EXIT_FAILURE); 
			}
			
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = args[i + 1] = NULL;
            return;
        }
    }
}

/* ── Pipe ────────────────────────────────────────────────── */

int run_piped_commands(char **args) 
{
    int pipe_pos = -1;
    for (int i = 0; args[i] != NULL; i++) 
	{
        if (strcmp(args[i], "|") == 0) 
		{ 
			pipe_pos = i; 
			break; 
		}
    }
    if (pipe_pos == -1) return 0;

    char *left[MAX_ARGS], *right[MAX_ARGS];

    for (int i = 0; i < pipe_pos; i++)
        left[i] = args[i];
    
	left[pipe_pos] = NULL;

    int j = 0;
    for (int i = pipe_pos + 1; args[i] != NULL; i++)
        right[j++] = args[i];
    
	right[j] = NULL;

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) 
	{
        perror(RED "  pipe()" RESET);
        return 1;
    }

    pid_t left_pid = fork();
    if (left_pid == 0) 
	{
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        
		if (execvp(left[0], left) == -1) 
		{
            printf(RED "  Command not found: %s\n" RESET, left[0]);
            exit(EXIT_FAILURE);
        }
    }

    pid_t right_pid = fork();
    if (right_pid == 0) 
	{
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[1]);
        close(pipe_fd[0]);
        
		if (execvp(right[0], right) == -1) 
		{
            printf(RED "  Command not found: %s\n" RESET, right[0]);
            exit(EXIT_FAILURE);
        }
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);
    waitpid(left_pid,  NULL, 0);
    waitpid(right_pid, NULL, 0);
    return 1;
}

/* ── Command Execution ───────────────────────────────────── */

void execute_command(char *input, int run_in_background) 
{
    char   buf[MAX_INPUT_LEN];
    char  *args[MAX_ARGS];
    int    dummy_bg;

    strcpy(buf, input);
    parse_args(buf, args, &dummy_bg);
    if (args[0] == NULL) return;

    if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) 
	{
        cmd_exit(args);
        return;
    }
    if (strcmp(args[0], "history") == 0) 
	{
        show_history();
        return;
    }
    if (strcmp(args[0], "!!") == 0) 
	{
        char *last = get_last_command();
        if (last == NULL) 
		{
            printf(YELLOW "  No commands in history.\n" RESET);
        } 
		else 
		{
            printf(CYAN "  Repeating: %s\n" RESET, last);
            char repeat_buf[MAX_INPUT_LEN];
            strcpy(repeat_buf, last);
            execute_command(repeat_buf, 0);
        }
        return;
    }
    if (strcmp(args[0], "help") == 0) 
	{
        cmd_help(args);
        return;
    }
    if (strcmp(args[0], "cd") == 0) 
	{
        cmd_cd(args);
        return;
    }

    char  pipe_buf[MAX_INPUT_LEN];
    char *pipe_args[MAX_ARGS];
    strcpy(pipe_buf, input);
    parse_args(pipe_buf, pipe_args, &dummy_bg);

    int has_pipe = 0;
    for (int i = 0; pipe_args[i] != NULL; i++) 
	{
        if (strcmp(pipe_args[i], "|") == 0) 
		{ 
			has_pipe = 1; break; 
		}
    }
    if (has_pipe) 
	{
        run_piped_commands(pipe_args);
        return;
    }

    pid_t child_pid = fork();

    if (child_pid == 0) 
	{
        char  exec_buf[MAX_INPUT_LEN];
        char *exec_args[MAX_ARGS];
        strcpy(exec_buf, input);
        parse_args(exec_buf, exec_args, &dummy_bg);

        setup_redirection(exec_args);

        if (execvp(exec_args[0], exec_args) == -1) 
		{
            printf(RED "  Command not found: '%s'\n" RESET, exec_args[0]);
            exit(EXIT_FAILURE);
        }

    } 
	else if (child_pid > 0) 
	{
        if (run_in_background == 0)
            waitpid(child_pid, NULL, 0);
        else
            printf(CYAN "  [Background PID: %d]\n" RESET, child_pid);
    } 
	else 
	{
        perror(RED "  fork()" RESET);
    }
}

/* ── Main ────────────────────────────────────────────────── */

int main(void) 
{
    char raw_input[MAX_INPUT_LEN];
    int  run_in_background;

    show_banner();

    while (shell_running) 
	{
        printf(GREEN "%s" RESET CYAN "> " RESET, get_prompt());
        fflush(stdout);

        if (fgets(raw_input, MAX_INPUT_LEN, stdin) == NULL) 
		{
            printf("\n");
            break;
        }

        strip_newline(raw_input);
        if (strlen(raw_input) == 0) continue;

        save_to_history(raw_input);

        char exec_input[MAX_INPUT_LEN];
        strcpy(exec_input, raw_input);

        run_in_background = 0;
        int input_len = strlen(exec_input);
        if (input_len > 0 && exec_input[input_len - 1] == '&') 
		{
            run_in_background         = 1;
            exec_input[input_len - 1] = '\0';
            input_len                 = strlen(exec_input);
            
			if (input_len > 0 && exec_input[input_len - 1] == ' ')
                exec_input[input_len - 1] = '\0';
        }

        execute_command(exec_input, run_in_background);
    }

    return 0;
}