**MyShell — A Custom Unix Shell**

A custom Unix-like shell built in C as part of an Operating Systems course project.
MyShell simulates the core functionality of standard shells like bash or sh, featuring
a colorized interface with a live timestamp prompt.

> Single-file C implementation — no external dependencies required.

---

**Project Overview**

| | |
|---|---|
| **Project Title** | MyShell — A Custom Unix Shell |
| **Language** | C |
| **Course** | Operating Systems |


**Features**

| | |
|---|---|
| **Colorized Prompt** | Live timestamp (HH:MM:SS) + current working directory |
| **Welcome Banner** | ASCII art banner with personalized username on startup |
| **Command History** | Circular buffer storing last 10 commands |
| **Repeat Command** | `!!` repeats the most recently executed command |
| **I/O Redirection** | Supports `>`, `>>`, and `<` operators |
| **Piping** | Single-level pipe using `\|` operator |
| **Background Execution** | Run commands in background using `&` |
| **Built-in Commands** | `cd`, `history`, `!!`, `help`, `exit`, `quit` |

---

**Built-in Commands**

| **Command** | **Description** |
|---|---|
| `cd <dir>` | Change current working directory |
| `cd` | Go to home directory |
| `history` | Display last 10 commands |
| `!!` | Repeat last command |
| `cmd > file` | Redirect output to file (overwrite) |
| `cmd >> file` | Redirect output to file (append) |
| `cmd < file` | Redirect input from file |
| `cmd1 \| cmd2` | Pipe output of cmd1 to cmd2 |
| `cmd &` | Run command in background |
| `exit / quit` | Exit the shell gracefully |

---

**System Calls Used**

| **System Call** | **Purpose** |
|---|---|
| `fork()` | Creates a child process to execute a command |
| `execvp()` | Replaces child process image with the new command |
| `waitpid()` | Parent waits for a specific child process to finish |
| `pipe()` | Creates a unidirectional pipe between two processes |
| `dup2()` | Redirects file descriptors for I/O redirection and piping |
| `open()` | Opens files for reading or writing during redirection |
| `close()` | Closes file descriptors after use to prevent leaks |
| `getcwd()` | Retrieves current working directory for the prompt |
| `chdir()` | Changes current working directory (used by cd) |
| `getenv()` | Reads environment variables like USER and HOME |

---

**Code Structure**

| **Function** | **Description** |
|---|---|
| `show_banner()` | Displays ASCII art welcome banner on startup |
| `get_prompt()` | Returns formatted prompt string with time and cwd |
| `save_to_history()` | Saves command to circular history buffer |
| `show_history()` | Prints all saved commands with numbered indices |
| `get_last_command()` | Returns most recently saved command |
| `strip_newline()` | Removes trailing newline from input string |
| `parse_args()` | Tokenizes input into argument array, detects `&` |
| `cmd_cd()` | Implements the cd built-in command |
| `cmd_help()` | Prints formatted help menu |
| `cmd_exit()` | Sets shell_running flag to 0, exits shell |
| `setup_redirection()` | Sets up file descriptors for I/O redirection |
| `run_piped_commands()` | Forks two processes connected via a pipe |
| `execute_command()` | Main dispatcher: built-ins, pipes, or fork+exec |
| `main()` | Shell loop: prompt → input → parse → execute |

---

**How to Compile & Run**

```bash
# Compile
gcc MyShell.c -o myshell

# Run
./myshell
```

**Example Commands**

| **Command** | **What it does** |
|---|---|
| `ls -la` | List files in detail |
| `ls \| grep .c` | Pipe — filter .c files |
| `ls > output.txt` | Redirect output to file |
| `cat < output.txt` | Read input from file |
| `sleep 5 &` | Run in background |
| `history` | View command history |
| `!!` | Repeat last command |

---

**License**

Academic project — all documentation is original work produced by the team.
