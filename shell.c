#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>

/// writes messages to shell (or other destination) and adds 
/// newline if message does not already end in one
void shellPrint(char *msg, int dest)
{
    write(dest, msg, strlen(msg));
    if (msg[strlen(msg) - 1] != '\n') {
        write(dest, "\n", 1);
    }
}


/// shell prompt for user
void shellPrompt() {
    char *cwd = getcwd(NULL, 0);
    char* username = getenv("USER");
    write(STDOUT_FILENO, username, strlen(username)); 
    write(STDOUT_FILENO, "@theCoolestShell:~", 18); 
    write(STDOUT_FILENO, cwd, strlen(cwd)); 
    write(STDOUT_FILENO, ">  ", 4); 
}


/// make completely new string
char *copy_string(char *str) {
    char *res = (char*)malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(res, str);
    return res;
}


/// removes whitespace on beginning by moving pointer and removes
///     newline and whitespace at end by replacing with \0
char *strip(char *ogmsg) {
    // printf("STARTog\n\n%s\n\nEND\n", ogmsg);    
    int i = 0;
    char* res = NULL;
    while (ogmsg[i]) {
        if (ogmsg[i] != ' ' && ogmsg[i] != '\t' && !res) {
            res = &ogmsg[i];
        }
        i++;
    }

    for (int j = i - 1; j >= 0; j--) {
        if (ogmsg[j] != ' ' && ogmsg[j] != '\n' && ogmsg[j] != '\t') {
            ogmsg[j + 1] = '\0';
            break;
        }
    }
    // printf("STARTres\n\n%s\n\nEND\n", res);
    return res ? res : ogmsg;
}


/// checks that an input begins with a certain cmd (whitespace needed)
bool starts_with_cmd(char* str, char* pre) {
    if (strlen(str) < strlen(pre)) {
        return false;
    }
    int i = 0;
    while (str[i] && pre[i]) {
        if (str[i] != pre[i]) {
            return false;
        }
        i++;
    }
    return str[i] == ' '  || str[i] == '\n' || str[i] == '\0';
}


/// checks that an input begins with a certain phrase
bool starts_with(char* str, char* pre) {
    if (strlen(str) < strlen(pre)) {
        return false;
    }
    int i = 0;
    while (str[i] && pre[i]) {
        if (str[i] != pre[i]) {
            return false;
        }
        i++;
    }
    return true;
}


/// returns an array which delimits by all chars in given string,
///     similar to .split() in python.
char **split(char *args, char *delimiters) {
    int c = 0;
    char *argscpy = copy_string(args);
    while ((strtok_r(argscpy, delimiters, &argscpy))) { 
        c++;
    }

    argscpy = copy_string(args);
    char **res = (char**)malloc(sizeof(char*) * (c + 1));
    for (int i = 0; i < c; i++) { 
        res[i] = strtok_r(argscpy, delimiters, &argscpy);
        // printf("%d: %s\n", i, res[i]);
    }
    return res;
}


/// returns length of array created by split
int splitlen(char *args, char *delimiters) {
    int c = 0;
    char *argscpy = copy_string(args);
    while ((strtok_r(argscpy, delimiters, &argscpy))) { 
        c++;
    }
    return c + 1;
}


/// cut string off at first occurence of token
void cut_string(char *str, char token) {
    int i = 0;
    while (str[i]) {
        if (str[i] == token) {
            str[i] = '\0';
            return;
        }
        i++;
    }
}


/// execute singular command in shell
void shellExec(char *arg, int dest) {
    char *raw_input = copy_string(arg);
    char *input = strip(raw_input);
    
    // empty input
    if (!input[0] || (input[0] == '\n') || (input[0] == '\t') || (input[0] == ' ')) {
        return;
    }
    
    // quit shell
    else if (strcmp(input, "exit") == 0) {
        exit(0);
    } 
    
    // pwd cmd
    else if (starts_with_cmd(input, "pwd")) {
        strtok_r(input, " \t", &input);
        char *post_pwd = strtok_r(input, " \t", &input);
        if (post_pwd  && strcmp(post_pwd, "\n") != 0) {
            shellPrint("Invalid args for 'pwd'", dest);
            return;
        }
        char *cwd = getcwd(NULL, 0);
        shellPrint(cwd, dest);
    }

    // cd cmd (no args)
    else if (strcmp(input, "cd") == 0) {
        chdir(getenv("HOME"));
    }

    // cd cmd (w args)
    else if (starts_with_cmd(input, "cd")) {
        char **args = split(input, " \t");
        int argslen = splitlen(input, " \t");
        if (argslen != 3) { 
            shellPrint("Invalid args for 'cd'", STDOUT_FILENO);
        } else {
            int res = chdir(args[1]);
            if (res != 0) {
                shellPrint("Invalid directory", STDOUT_FILENO);
            }
            return;
        }
    }

    // echo cmd
    else if (starts_with_cmd(input, "echo")) {
        shellPrint(strip(input + 4), dest);
    }

    // exec cmds and invalid inputs
    else {
        __pid_t pid = fork();

        if (pid < 0) {
            fprintf(stderr, "Child could not be created.\n");
            exit(1);
        } 
        
        // parent process
        else if (pid > 0) {
            int status = 0;
            waitpid(pid, &status, 0);
            // fprintf(stderr, "status: %d\n", status);
            // fprintf(stderr, "dest: %d\n", dest);
            if (status == 1 << 8) { 
                shellPrint("Invalid command", dest);
            }
        } 
        
        // child process
        else {
            dup2(dest, STDOUT_FILENO);
            char **argv = split(input, " \n");
            execvp(strtok_r(input, " \t", &input), argv);

            // shouldn't come here unless execvp cmd was invalid
            exit(1);
        }
    }
}


int main(int argc, char *argv[]) 
{
    FILE *file = NULL;
    if (argc == 2) {
        file = fopen(argv[1], "r");
        if (!file) {
            fprintf(stderr, "file couldn't be opened\n");
            exit(1);
        }
    } else if (argc > 2) {
        fprintf(stderr, "too many arguments for shell\n");
        exit(1);
    }

    char cmd_buff[514];
    char *cmds;

    // shell mode
    if (!file) {
        while (true) {
            shellPrompt();
            cmds = fgets(cmd_buff, 514, stdin);
            if (!cmds) {
                exit(0);
            } 

            // cmd too big
            else if (!strchr(cmds, '\n')) {
                write(STDOUT_FILENO, cmds, strlen(cmds));
                while ((cmds = fgets(cmd_buff, 514, file))) {
                    write(STDOUT_FILENO, cmds, strlen(cmds));
                    if (strchr(cmds, '\n')) {
                        break;
                    }
                }
                shellPrint("Command is too large (max: 512 characters)", STDOUT_FILENO);
                continue;
            }
            
            char *cmd;
            bool advred = false;
            while ((cmd = strtok_r(cmds, ";", &cmds))) {

                // ignore if builtin cmd
                if (starts_with(cmd, "cd") || starts_with(cmd, "exit")) {
                    shellExec(cmd, STDOUT_FILENO);
                    continue;
                }

                // find output destination
                int dest = STDOUT_FILENO;
                char *cmdcpy = copy_string(cmd);
                char **cmdargs = split(cmd, ">");
                int cmdarglen = splitlen(cmd, ">");
                if (cmdarglen > 2) {
                    // invalid dest if extra stuff
                    strtok_r(cmdcpy, ">", &cmdcpy);
                    if (strchr(cmdcpy, '>')) {
                        shellPrint("Invalid redirection", STDOUT_FILENO);
                        continue;
                    }
                    // see if dest can be written to and check for '+'
                    char* destfile = strip(cmdargs[1]);
                    if (destfile[0] == '+') {
                        advred = true;
                        destfile++;
                    }
                    if (!destfile) {
                        shellPrint("Invalid redirection", STDOUT_FILENO);
                        continue;
                    }
                    destfile = strip(destfile);

                    // see what do do in terms of redirection
                    if (access(destfile, F_OK) != 0) {
                        FILE *output_dest = fopen(destfile, "w");
                        if (!output_dest) {
                            shellPrint("Cannot open filepath", STDOUT_FILENO);
                            continue;
                        }
                        dest = fileno(output_dest);
                    } else if (!advred) {
                        shellPrint("File already exists! To prepend, use '>+'", STDOUT_FILENO);
                        continue;
                    } else {
                        // write to temp file
                        FILE *output_dest = fopen("i_dont_want_overlap_with_known_files_tmp", "w");
                        dest = fileno(output_dest);
                        cut_string(cmd, '>');
                        shellExec(cmd, dest);
                        // reopen temp file for append
                        FILE *output_append = fopen("i_dont_want_overlap_with_known_files_tmp", "a");
                        dest = fileno(output_append);
                        // open og file
                        FILE *ogfile = fopen(destfile, "r");
                        char* transfer_buff = (char*)malloc(sizeof(char) * 10);
                        while ((transfer_buff = fgets(transfer_buff, 10, ogfile))) {
                            write(dest, transfer_buff, strlen(transfer_buff));
                        }
                        // rename appended file to replace
                        rename("i_dont_want_overlap_with_known_files_tmp", destfile);
                        free(transfer_buff);
                        break;
                    }
                }
                
                // remove destination token
                if (dest != STDOUT_FILENO) {
                    cut_string(cmd, '>');
                }
                
                // execute shell command
                shellExec(cmd, dest);
            }
        }
    } 
    
    // batch mode
    else  {
        while ((cmds = fgets(cmd_buff, 514, file))) {
            if (!strchr(cmds, '\n')) {
                write(STDOUT_FILENO, cmds, strlen(cmds));
                while ((cmds = fgets(cmd_buff, 514, file))) {
                    write(STDOUT_FILENO, cmds, strlen(cmds));
                    if (strchr(cmds, '\n')) {
                        break;
                    }
                }
                shellPrint("Command is too large (max: 512 characters)", STDOUT_FILENO);
                continue;
            }

            char *cmdscpy = copy_string(cmds);
            char *input = strip(cmdscpy);
            if (input[0] && (input[0] != '\n')) {
                write(1, "COMMAND : ", 10);
                shellPrint(cmds, STDOUT_FILENO);
            }
            
            char *cmd;
            bool advred = false;
            while ((cmd = strtok_r(cmds, ";", &cmds))) {

                // ignore if builtin cmd
                if (starts_with(cmd, "cd") || starts_with(cmd, "exit") || starts_with(cmd, "pwd")) {
                    shellExec(cmd, STDOUT_FILENO);
                    continue;
                }

                // find output destination
                int dest = STDOUT_FILENO;
                char *cmdcpy = copy_string(cmd);
                char **cmdargs = split(cmd, ">");
                int cmdarglen = splitlen(cmd, ">");
                if (cmdarglen > 2) {
                    // invalid dest if extra stuff
                    strtok_r(cmdcpy, ">", &cmdcpy);
                    if (strchr(cmdcpy, '>')) {
                        shellPrint("Invalid redirection", STDOUT_FILENO);
                        continue;
                    }
                    // see if dest can be written to and check for '+'
                    char* destfile = strip(cmdargs[1]);
                    if (destfile[0] == '+') {
                        advred = true;
                        destfile++;
                    }
                    if (!destfile) {
                        shellPrint("Invalid redirection", STDOUT_FILENO);
                        continue;
                    }
                    destfile = strip(destfile);

                    // see what do do in terms of redirection
                    if (access(destfile, F_OK) != 0) {
                        FILE *output_dest = fopen(destfile, "w");
                        if (!output_dest) {
                            shellPrint("Cannot open filepath", STDOUT_FILENO);
                            continue;
                        }
                        dest = fileno(output_dest);
                    } else if (!advred) {
                        shellPrint("File already exists! To prepend, use '>+'", STDOUT_FILENO);
                        continue;
                    } else {
                        // write to temp file
                        FILE *output_dest = fopen("i_dont_want_overlap_with_known_files_tmp", "w");
                        dest = fileno(output_dest);
                        cut_string(cmd, '>');
                        shellExec(cmd, dest);
                        // reopen temp file for append
                        FILE *output_append = fopen("i_dont_want_overlap_with_known_files_tmp", "a");
                        dest = fileno(output_append);
                        // open og file
                        FILE *ogfile = fopen(destfile, "r");
                        char* transfer_buff = (char*)malloc(sizeof(char) * 10);
                        while ((transfer_buff = fgets(transfer_buff, 10, ogfile))) {
                            write(dest, transfer_buff, strlen(transfer_buff));
                        }
                        // rename appended file to replace
                        rename("i_dont_want_overlap_with_known_files_tmp", destfile);
                        free(transfer_buff);
                        break;
                    }
                }
                
                // remove destination token
                if (dest != STDOUT_FILENO) {
                    cut_string(cmd, '>');
                }
                
                // execute shell command
                shellExec(cmd, dest);
            }
        }
    }

    if (argc >= 2) {
        fclose(file);
    }
    return 0;
}
