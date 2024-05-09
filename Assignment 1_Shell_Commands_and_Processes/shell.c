// Shell starter file
// You may make any changes to any part of this file.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <wait.h>
#include <signal.h>
#include <pwd.h>
#include <error.h>
#include <errno.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)

// store a list of history command
#define HISTORY_DEPTH 10
struct history_entry
{
    int idx;        // command index
    char cmd[1000]; // command string
};
struct history_entry history[HISTORY_DEPTH];

// utils function
// write a string into terminal by write function
void write_msg(const char* msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

/**
 * Command Input and Processing
 */

 /*
  * Tokenize the string in 'buff' into 'tokens'.
  * buff: Character array containing string to tokenize.
  *       Will be modified: all whitespace replaced with '\0'
  * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
  *       Will be modified so tokens[i] points to the i'th token
  *       in the string buff. All returned tokens will be non-empty.
  *       NOTE: pointers in tokens[] will all point into buff!
  *       Ends with a null pointer.
  * returns: number of tokens.
  */
int tokenize_command(char* buff, char* tokens[])
{
    int token_count = 0;
    _Bool in_token = false;
    int num_chars = strnlen(buff, COMMAND_LENGTH);
    for (int i = 0; i < num_chars; i++)
    {
        switch (buff[i])
        {
            // Handle token delimiters (ends):
        case ' ':
        case '\t':
        case '\n':
            buff[i] = '\0';
            in_token = false;
            break;

            // Handle other characters (may be start)
        default:
            if (!in_token)
            {
                tokens[token_count] = &buff[i];
                token_count++;
                in_token = true;
            }
        }
    }
    tokens[token_count] = NULL;
    return token_count;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of
 * tokens). in_background: pointer to a boolean variable. Set to true if user
 * entered an & as their last token; otherwise set to false.
 */
void process_command(char* buff, char* tokens[], _Bool* in_background)
{
    *in_background = false;

    // Tokenize (saving original command string)
    int token_count = tokenize_command(buff, tokens);
    if (token_count == 0)
    {
        return;
    }

    // Extract if running in background:
    if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0)
    {
        *in_background = true;
        tokens[token_count - 1] = 0;
    }
}

// read command string from console
// handle history-series command
int read_command(char input_buffer[COMMAND_LENGTH])
{
    int i;
    char display_buf[COMMAND_LENGTH];

    // show current directory
    getcwd(display_buf, COMMAND_LENGTH);
    write_msg(display_buf);
    write_msg("$ ");

    // Read input
    memset(input_buffer, 0, COMMAND_LENGTH - 1);
    int length = read(STDIN_FILENO, input_buffer, COMMAND_LENGTH - 1);

    if ((length < 0) && (errno != EINTR))
    {
        perror("Unable to read command. Terminating.\n");
        exit(-1);  // terminate with error
    }

    // strip \n.
    for (i = 0; input_buffer[i]; i++)
    {
        if (input_buffer[i] == '\r' || input_buffer[i] == '\n')
        {
            input_buffer[i] = '\0';
        }
    }

    if (input_buffer[0] == '\0')
    {
        // empty command
        return -1;
    }
    else if (input_buffer[0] == '!')
    {
        // (1) a history command
        if (input_buffer[1] == '\0')
        {
            write_msg("Error illegal command \"!\"\n");
            return 1;
        }
        else if (input_buffer[1] == '!')
        {
            // !!

            // If there is no previous command, display an error message.
            if (history[0].idx == 0)
            {
                write_msg("Error No previous command!\n");
                return 1;
            }
            else
            {
                strcpy(input_buffer, history[0].cmd);
                write_msg(input_buffer);
                write_msg("\n");

                // append to command history
                for (i = 9; i > 0; i--)
                {
                    history[i] = history[i - 1];
                }
                history[0].idx++;
                strcpy(history[0].cmd, input_buffer);
            }
        }
        else // a number
        {
            int idx = atoi(&input_buffer[1]);
            char numbuf[20];
            sprintf(numbuf, "%d", idx);

            if (strcmp(numbuf, &input_buffer[1]) != 0 || idx < 0)
            {
                write_msg("Error n is not a valid number!\n");
                return -1;
            }
            for (i = 0; i < HISTORY_DEPTH; i++)
            {
                if (history[i].idx - 1 == idx)
                {
                    // find command success
                    strcpy(input_buffer, history[i].cmd);
                    write_msg(input_buffer);
                    write_msg("\n");

                    // append to command history
                    for (i = 9; i > 0; i--)
                    {
                        history[i] = history[i - 1];
                    }
                    history[0].idx++;
                    strcpy(history[0].cmd, input_buffer);

                    return 0;
                }
            }
            write_msg("Error n is not a valid number!\n");
            return -1;
        }
    }
    else
    {
        // (2) a non-history command
        // add into command buffer

        // append to command history
        for (i = 9; i > 0; i--)
        {
            history[i] = history[i - 1];
        }
        history[0].idx++;
        strcpy(history[0].cmd, input_buffer);
    }
    return 0;
}

/*****************************************************************/
//               some build-in command
/*****************************************************************/
// process help command
void process_help(char* tokens[NUM_TOKENS])
{
    char display_buf[COMMAND_LENGTH];

    if (tokens[1] == NULL)
    {
        // If there is no argument provided, list all the supported internal commands
        write_msg("'cd' is a builtin command for changing the current working directory.\n");
        write_msg("'exit' is a builtin command for exit the shell program\n");
        write_msg("'pwd' is a builtin command for display the current working directory\n");
        write_msg("'help' is a builtin command for display help information on internal commands.\n");
        write_msg("'history' is a builtin command for displays the 10 most recent commands executed in the shell.\n");
    }
    else if (tokens[2] == NULL)
    {
        if (strcmp(tokens[1], "cd") == 0)
        {
            write_msg("'cd' is a builtin command for changing the current working directory.\n");
        }
        else if (strcmp(tokens[1], "exit") == 0)
        {
            write_msg("'exit' is a builtin command for exit the shell program\n");
        }
        else if (strcmp(tokens[1], "pwd") == 0)
        {
            write_msg("'pwd' is a builtin command for display the current working directory\n");
        }
        else if (strcmp(tokens[1], "help") == 0)
        {
            write_msg("'help' is a builtin command for display help information on internal commands.\n");
        }
        else if (strcmp(tokens[1], "history") == 0)
        {
            write_msg("'history' is a builtin command for displays the 10 most recent commands executed in the shell.\n");
        }
        else
        {
            sprintf(display_buf, "'%s' is an external command or application\n", tokens[1]);
            write_msg(display_buf);
        }
    }
    else
    {
        // If there is more than one argument, display an error message.
        write_msg("Error help command have 0 or 1 argument!\n");
    }
}

// process cd command
void process_cd(char* tokens[NUM_TOKENS], char prev_path[COMMAND_LENGTH])
{
    // get home path
    struct passwd* pw = getpwuid(getuid());
    char home_path[1024];
    char tmp[1024];
    strcpy(home_path, pw->pw_dir);

    // process `cd` command
    if (tokens[1] == NULL)
    {
        tokens[1] = home_path;
        tokens[2] = NULL;
    }

    if (tokens[2] != NULL)
    {
        write_msg("Error cd command have one argument!\n");
    }
    else
    {
        if (strcmp(tokens[1], "-") == 0)
        {
            // previous
            tokens[1] = prev_path;
        }
        else if (tokens[1][0] == '~')
        {
            strcat(home_path, &tokens[1][1]);
            tokens[1] = home_path;
        }

        getcwd(tmp, COMMAND_LENGTH);
        if (chdir(tokens[1]))
        {
            write_msg("Error cd command fail!\n");
        }
        else
        {
            strcpy(prev_path, tmp);
        }
    }
}


// Ctrl-C signal
void signal_handler()
{
    char* tokens[NUM_TOKENS];
    tokens[0] = "help";
    tokens[1] = 0;
    write_msg("\n");
    process_help(tokens);
}

/**
 * Main and Execute Commands
 */
int main(int argc, char* argv[])
{
    int i, ret;
    char display_buf[COMMAND_LENGTH];

    // set initial previous path
    char prev_path[COMMAND_LENGTH];
    getcwd(prev_path, COMMAND_LENGTH);

    // clear history
    memset(history, 0, sizeof(struct history_entry) * HISTORY_DEPTH);

    // register a custom signal handler for the SIGINT signal.
    struct sigaction handler;
    handler.sa_handler = signal_handler;
    handler.sa_flags = 0;
    sigemptyset(&handler.sa_mask);
    sigaction(SIGINT, &handler, NULL);


    char input_buffer[COMMAND_LENGTH];
    char* tokens[NUM_TOKENS];
    while (true)
    {
        // Get command
        // Use write because we need to use read() to work with
        // signals, and read() is incompatible with printf().

        ret = read_command(input_buffer);
        if (ret != 0)
        {
            // user input an illegal command
            continue;
        }

        _Bool in_background = false;
        process_command(input_buffer, tokens, &in_background);

        // DEBUG: Dump out arguments:
        //for (int i = 0; tokens[i] != NULL; i++)
        //{
        //    write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
        //    write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
        //    write(STDOUT_FILENO, "\n", strlen("\n"));
        //}
        //if (in_background)
        //{
        //    write(STDOUT_FILENO, "Run in background.", strlen("Run in background."));
        //}

        /**
         * Steps For Basic Shell:
         * 1. Fork a child process
         * 2. Child process invokes execvp() using results in token array.
         * 3. If in_background is false, parent waits for
         *    child to finish. Otherwise, parent loops back to
         *    read_command() again immediately.
         */

        if (strcmp(tokens[0], "exit") == 0)
        {
            // (1) Exit the shell program
            if (tokens[1] != NULL)
            {
                //write_msg("[Error] `exit` command do not have argument!\n");
                write_msg("exit command do not have argument!\n");
            }
            else
            {
                break;
            }
        }
        else if (strcmp(tokens[0], "pwd") == 0)
        {
            // (2) Display the current working directory
            if (tokens[1] != NULL)
            {
                write_msg("Error `pwd` command do not have argument!\n");
            }
            else
            {
                getcwd(display_buf, COMMAND_LENGTH);
                write_msg(display_buf);
                write_msg("\n");
            }
        }
        else if (strcmp(tokens[0], "cd") == 0)
        {
            // (3) Change the current working directory
            process_cd(tokens, prev_path);
        }
        else if (strcmp(tokens[0], "help") == 0)
        {
            // (4) Display help information on internal commands.
            process_help(tokens);

        }
        else if (strcmp(tokens[0], "history") == 0)
        {
            // (3) displays the 10 most recent commands executed in the shell
            for (i = 0; i < HISTORY_DEPTH; i++)
            {
                if (history[i].idx == 0)
                {
                    break;
                }
                sprintf(display_buf, "%d\t%s\n", history[i].idx - 1, history[i].cmd);
                write_msg(display_buf);
            }
        }
        else
        {
            pid_t pid = fork();
            assert(pid >= 0);
            if (pid == 0)
            {
                // child
                execvp(tokens[0], tokens);
                // returns an error (see man execvp) then display an error message
                write_msg("Error execvp fail!\n");
                exit(0);
            }
            else
            {
                // parent
                if (!in_background)
                {
                    // wait for child to exit
                    int status;
                    waitpid(pid, &status, 0);
                }
            }
        }
    }
    return 0;
}

