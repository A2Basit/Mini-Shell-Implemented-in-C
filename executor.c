#include <stdio.h>
#include <stdlib.h>
#include "command.h"
#include "executor.h"
#include <sysexits.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>

static int execute_aux(struct tree *t, int o_input_fd, int p_output_fd);

int customcommand()
{
   printf("This is the custom command");
   return 0;
}

#define MAX_COMMAND_LENGTH 100
#define MAX_HISTORY_SIZE 100
char command[MAX_COMMAND_LENGTH];

int lineCount = 1;

void saveCommand(const char *command)
{
   FILE *historyFile = fopen("command_history.txt", "a");
   if (historyFile == NULL)
   {
      printf("Error opening command history file.\n");
      return;
   }

   fprintf(historyFile, "%s\n", command);
   fclose(historyFile);
}

void printCommandHistory()
{
   FILE *historyFile = fopen("command_history.txt", "r");
   if (historyFile == NULL)
   {
      printf("No command history found.\n");
      return;
   }
   while (fgets(command, sizeof(command), historyFile) != NULL)
   {
      printf("%d. %s", lineCount, command);
      lineCount++;
   }

   fclose(historyFile);
}

/*print working directory*/
int printWorkingDirectory()
{
   char cwd[1024];
   if (getcwd(cwd, sizeof(cwd)) != NULL)
   {
      printf("Current working directory: %s\n", cwd);
      return 0;
   }
   else
   {
      perror("Failed to get current working directory");
      return -1;
   }
}
/*declare global inorder to avoid mix declartions*/
ssize_t len;
/*process management*/
int psCommand()
{
   pid_t pid;
   DIR *dir;
   struct dirent *entry;
   char path[1024];
   char exePath[1024];

   printf("PID   COMMAND\n");

   if ((dir = opendir("/proc")) == NULL)
   {
      perror("opendir");
      return -1;
   }

   while ((entry = readdir(dir)) != NULL)
   {
      pid = atoi(entry->d_name);
      if (pid > 0)
      {
         sprintf(path, "/proc/%d/exe", pid);

         len = readlink(path, exePath, sizeof(exePath) - 1);
         if (len != -1)
         {
            exePath[len] = '\0';
            printf("%d    %s\n", pid, exePath);
         }
      }
   }

   closedir(dir);

   return 0;
}

/*Clear the console*/
int clearConsole()
{
   system("clear");
   return 0;
}
/*Touch command*/
int touchFile(const char *filename)
{
   FILE *file = fopen(filename, "w");
   if (file == NULL)
   {
      printf("Error creating file: %s\n", filename);
      return 1;
   }
   fclose(file);
   return 0;
}
/*Remove directory*/

int rmdirCommand(const char *directory)
{
   if (rmdir(directory) == 0)
   {
      printf("Directory %s removed successfully.\n", directory);
      return 0;
   }
   else
   {
      perror("Failed to remove directory");
      return -1;
   }
}

int removefile()
{
   char filename[100];

   printf("Enter the name of the file to be deleted: ");
   scanf("%s", filename);

   if (unlink(filename) == 0)
   {
      printf("File '%s' successfully deleted.\n", filename);
   }
   else
   {
      printf("Error deleting the file '%s'.\n", filename);
   }
}
int renameFile(const char *oldname, const char *newname)
{
   if (rename(oldname, newname) == 0)
   {
      printf("File renamed successfully.\n");
      return 0;
   }
   else
   {
      perror("Failed to rename file");
      return -1;
   }
}
int mvcommand()
{
   char sourceFilename[100];
   char destinationFilename[100];
   FILE *sourceFile, *destinationFile;
   char ch;

   printf("Enter the name of the source file: ");
   scanf("%s", sourceFilename);

   printf("Enter the name of the destination file: ");
   scanf("%s", destinationFilename);
   sourceFile = fopen(sourceFilename, "r");
   if (sourceFile == NULL)
   {
      printf("Error opening the source file.\n");
      return 1;
   }

   destinationFile = fopen(destinationFilename, "w");
   if (destinationFile == NULL)
   {
      printf("Error opening the destination file.\n");
      fclose(sourceFile);
      return 1;
   }
   while ((ch = fgetc(sourceFile)) != EOF)
   {
      fputc(ch, destinationFile);
   }

   printf("File contents moved successfully.\n");

   fclose(sourceFile);
   fclose(destinationFile);
   if (remove(sourceFilename) == 0)
   {
      printf("Source file '%s' removed.\n", sourceFilename);
   }
   else
   {
      printf("Error removing the source file '%s'.\n", sourceFilename);
   }

   return 0;
}
int mkdirCommand(const char *directory)
{
   int status = mkdir(directory, 0777);
   if (status == 0)
   {
      printf("Directory %s created successfully.\n", directory);
      return 0;
   }
   else
   {
      if (errno == EEXIST)
      {
         printf("Directory %s already exists.\n", directory);
         return 0;
      }
      else
      {
         perror("Failed to create directory");
         return -1;
      }
   }
}
int execute(struct tree *t)
{
   execute_aux(t, STDIN_FILENO, STDOUT_FILENO); /*processing the root node first*/
   return 0;
}

static int execute_aux(struct tree *t, int p_input_fd, int p_output_fd)
{
   int pid, status, new_pid, fd, fd2, success_status = 0, failure_status = -1, new_status;
   int pipe_fd[2], pid_1, pid_2;

   /* none conjunction processing */
   if (t->conjunction == NONE)
   {
      /* Check if exit command was entered, exit if it was */
      if (strcmp(t->argv[0], "exit") == 0)
      {
         exit(0);
      }

      /* Check if user wants to change directory, if so change directory */
      else if (strcmp(t->argv[0], "cd") == 0)
      {
         char cwd[1024]; /*Buffer to store the current directory path*/

         if (t->argv[1] != NULL)
         {
            if (chdir(t->argv[1]) == -1)
            {
               perror("chdir");
               return -1;
            }
         }
         else
         {
            if (chdir(getenv("HOME")) == -1) /* Change to home directory if no argument is provided */
            {
               perror("chdir");
               return -1;
            }
         }
         /*Print the current working directory*/
         if (getcwd(cwd, sizeof(cwd)) != NULL)
         {
            printf("Current working directory: %s\n", cwd);
         }
         else
         {
            perror("getcwd");
            return -1;
         }

         return 0;
      }

      /*display a list of available commands*/
      else if (strcmp(t->argv[0], "help") == 0)
      {
         printf("Available commands:\n");
         printf("1. exit - Exit the shell.\n");
         printf("2. cd [directory] - Change the current directory.\n");
         printf("3. help - Display a list of available commands.\n");
         printf("4. date - Display the current date and time.\n");
         printf("5. echo [arguments] - Print the given arguments.\n");
         printf("6. list - List files in the current directory.\n");
         printf("7. pwd - Print the working directory.\n");
         printf("8. clear - Clear the console.\n");
         printf("9. touch [filename] - Create a new file.\n");
         printf("10. ps - Display a list of running processes.\n");
         printf("11. rmdir [directory] - Remove a directory.\n");
         printf("12. mkdir [directory] - Create a new directory.\n");
         printf("13. input.txt << sum.c >> output.txt - uses input and output redirection so that sum.c can execute using input from the input.txt file and output its results to the output.txt file|.\n");
         printf("14. random_numbers.c | sum.c - the standard output of a program called randon_numbers.c will be connected to the standard input for the sum.c program, thus creating a pipeline.\n");
         printf("15. remove [enter] Then enter the [filename] - Remove a file.\n");
         printf("16. rename [oldname] [newname] - Rename a file.\n");
         printf("17. mv [source] [destination] - Move content of one file to other.\n");
         printf("18. history command:\n");
      }

      /*display the current date and time*/
      else if (strcmp(t->argv[0], "date") == 0)
      {
         time_t rawtime;
         struct tm *timeinfo;
         time(&rawtime);
         timeinfo = localtime(&rawtime);
         printf("Current date and time: %s", asctime(timeinfo));
      }
      /*move content of one file to other*/
      else if (strcmp(t->argv[0], "mv") == 0)
      {
         return mvcommand();
      }
      else if (strcmp(t->argv[0], "history") == 0)
      {
         printf("Command history:\n");
         printCommandHistory();
         
      }
      /*rename a file*/
      else if (strcmp(t->argv[0], "rename") == 0)
      {
         if (t->argv[1] != NULL && t->argv[2] != NULL)
         {
            return renameFile(t->argv[1], t->argv[2]);
         }
         else
         {
            printf("Usage: rename [oldname] [newname]\n");
            return -1;
         }
      }

      /*echo to the console*/
      else if (strcmp(t->argv[0], "echo") == 0)
      {
         int i;
         for (i = 1; t->argv[i] != NULL; i++)
         {
            printf("%s ", t->argv[i]);
         }
         printf("\n");
      }

      /*remove a file*/
      else if (strcmp(t->argv[0], "remove") == 0)
      {
         return removefile();
      }

      /*list the directories and files*/
      else if (strcmp(t->argv[0], "list") == 0)
      {
         DIR *dir;
         struct dirent *entry;

         if ((dir = opendir(".")) == NULL)
         {
            perror("opendir");
            return failure_status;
         }

         while ((entry = readdir(dir)) != NULL)
         {
            printf("%s\n", entry->d_name);
         }

         closedir(dir);
      }

      /*custom command*/
      else if (strcmp(t->argv[0], "custom") == 0)
      {
         return customcommand();
      }

      /*print the working directory*/
      else if (strcmp(t->argv[0], "pwd") == 0)
      {
         return printWorkingDirectory();
      }
      /*clear the console*/
      else if (strcmp(t->argv[0], "clear") == 0)
      {
         return clearConsole();
      }

      /*touch command*/
      else if (strcmp(t->argv[0], "touch") == 0)
      {
         if (t->argv[1] != NULL)
         {
            return touchFile(t->argv[1]);
         }
         else
         {
            printf("No filename provided.\n");
            return failure_status;
         }
      }
      /* Process management */
      else if (strcmp(t->argv[0], "ps") == 0)
      {
         return psCommand();
      }
      /* mkdir command */
      else if (strcmp(t->argv[0], "mkdir") == 0)
      {
         if (t->argv[1] != NULL)
         {
            return mkdirCommand(t->argv[1]);
         }
         else
         {
            printf("No directory name provided.\n");
            return failure_status;
         }
      }

      /* rmdir command */
      else if (strcmp(t->argv[0], "rmdir") == 0)
      {
         if (t->argv[1] != NULL)
         {
            return rmdirCommand(t->argv[1]);
         }
         else
         {
            printf("No directory name provided.\n");
            return failure_status;
         }
      }

      return success_status;
      /*process any entered linux commands*/
      if ((pid = fork()) < 0)
      {
         perror("fork");
         exit(-1);
      }
      /*parent processing*/
      if (pid != 0)
      {
         wait(&status);
         return status;
      }

      /*child processing*/
      else
      {

         /*check if we have any input/output files*/
         if (t->input != NULL)
         {
            if ((fd = open(t->input, O_RDONLY)) < 0)
            {
               perror("fd");
               exit(-1);
            }
            /*get input from the file if it exists, and opening it has not failed*/
            if (dup2(fd, STDIN_FILENO) < 0)
            {
               perror("dup2");
               exit(-1);
            }

            /*close the file descriptor*/
            if (close(fd) < 0)
            {
               perror("close");
               exit(-1);
            }
         }

         /*use provided output file if it exists*/
         if (t->output != NULL)
         {
            if ((fd = open(t->output, O_CREAT | O_WRONLY | O_TRUNC)) < 0)
            {
               perror("open");
               exit(-1);
            }

            /*change standard output to get output from provided file if exists*/
            if (dup2(fd, STDOUT_FILENO) < 0)
            {
               perror("dup2");
               exit(-1);
            }

            /*close file descriptor associated with output file*/
            if (close(fd) < 0)
            {
               perror("close");
               exit(-1);
            }
         }

         /*execute the command using*/
         execvp(t->argv[0], t->argv);
         fprintf(stderr, "Failed to execute %s\n", t->argv[0]);
         exit(-1);
      }
   }
   else if (t->conjunction == AND)
   {

      /*Process left subtree, then right subtree if leftsubtree is processed correctly*/
      new_status = execute_aux(t->left, p_input_fd, p_output_fd);

      /*if the left_subtree was processed was succesfully, process the right subtree */
      if (new_status == 0)
      {
         return execute_aux(t->right, p_input_fd, p_output_fd);
      }
      else
      {
         return new_status;
      }
   }

   /*of the current conjuction is of type pipe, process*/
   else if (t->conjunction == PIPE)
   {

      if (t->left->output != NULL)
      {
         printf("Ambiguous output redirect.\n");
         return failure_status;
      }

      if (t->right->input != NULL)
      {
         printf("Ambiguous output redirect.\n");
         return failure_status;
      }

      /* create a pipe */
      if (pipe(pipe_fd) < 0)
      {
         perror("pipe");
         exit(-1);
      }

      /*fork(creating child process)*/
      if ((pid_1 = fork()) < 0)
      {
         perror("fork");
      }

      if (pid_1 == 0)
      { /*child 1  process code*/

         close(pipe_fd[0]); /*close read end since were not using it*/

         if (t->input != NULL)
         {

            if ((fd = open(t->input, O_RDONLY)) < 0)
            {
               perror("open");
               exit(-1);
            }

            /*make the pipes write end the new standard output*/
            dup2(pipe_fd[1], STDOUT_FILENO);

            /*pass in input file if exists and the pipes write end for the output file descriptor*/
            execute_aux(t->left, fd, pipe_fd[1]);

            /*closed pipe write end*/
            if (close(pipe_fd[1]) < 0)
            {
               perror("close");
               exit(-1);
            }

            /*close input file*/
            if (close(fd) < 0)
            {
               perror("close");
               exit(-1);
            }
         }
         /*if no input file was provided, use parent file descriptor as the input file*/
         else
         {
            dup2(pipe_fd[1], STDOUT_FILENO);
            execute_aux(t->left, p_input_fd, pipe_fd[1]); /*process the left subtree*/
            if (close(pipe_fd[1] < 0))
            {
               perror("close");
               exit(-1);
            }
         }
      }
      else
      {
         /*create second child to handle right subtree*/

         if ((pid_2 = fork()) < 0)
         {
            perror("fork");
            exit(-1);
         }

         if (pid_2 == 0)
         { /*child two code*/

            /*close write end of pipe, dont need it*/
            close(pipe_fd[1]);

            /*use output file if provided, else use parent output file descriptor*/
            if (t->output != NULL)
            {

               /*open provided output file if exists*/
               if ((fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC)) < 0)
               {
                  perror("open");
                  exit(-1);
               }

               /*make the pipes read end the new standard input*/
               dup2(pipe_fd[0], STDIN_FILENO);
               execute_aux(t->right, pipe_fd[0], fd); /*process the right subtree*/

               /*close pipe read end*/
               if (close(pipe_fd[0]) < 0)
               {
                  perror("close");
                  exit(-1);
               }

               /*closed output file*/
               if (close(fd) < 0)
               {
                  perror("close");
                  exit(-1);
               }
            }

            else
            {
               dup2(pipe_fd[0], STDIN_FILENO);
               execute_aux(t->right, pipe_fd[0], p_output_fd); /*process the right subtree*/
            }
         }
         else
         {

            /* Parent has no need for the pipe */
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            /* Reaping each child */
            wait(&status);
            wait(&status);
         }
      }
   }
   /*if current conjuction enumerator is of type subshell, process commands
     in a child process and return success/failure status after executing*/
   else if (t->conjunction == SUBSHELL)
   {
      /*fork(creating child process)*/
      if ((new_pid = fork()) < 0)
      {
         perror("fork");
         exit(-1);
      }

      /*parent code*/
      if (new_pid != 0)
      {
         wait(&status);             /*wait for child*/
         exit(WEXITSTATUS(status)); /*return the status*/
      }
      else
      {
         /*child code*/

         /*get input from input file if it exists*/
         if (t->input != NULL)
         {
            /*open input file*/
            if ((fd = open(t->input, O_RDONLY)) < 0)
            {
               perror("fd");
               exit(-1);
            }
            /*changed standard input to use input from input file*/
            if (dup2(fd, STDIN_FILENO) < 0)
            {
               perror("dup2");
               exit(-1);
            }
            /*close file descriptor(input file)*/
            if (close(fd) < 0)
            {
               perror("fd");
               exit(-1);
            }
         }
         /*if there was no input file use provided parent file descriptor(the file parameter of the function)*/
         else
         {
            fd = p_input_fd;
         }

         /*use an output file if it exists*/
         if (t->output != NULL)
         {
            if ((fd2 = open(t->output, O_WRONLY | O_CREAT | O_TRUNC)) < 0)
            {
               perror("fd");
               exit(-1);
            }

            /*change standard output to the output file(output will be written to output file)*/
            if (dup2(fd2, STDOUT_FILENO))
            {
               perror("dup2");
               exit(-1);
            }
            /*close the output file descriptor*/
            if (close(fd2) < 0)
            {
               perror("fd");
               exit(-1);
            }
         }
         /*if no outputfile exists write output to provided parent output file */
         else
         {
            fd2 = p_output_fd;
         }
         /*execute left subtree and get status*/
         status = execute_aux(t->left, fd, fd2);
         /*return with value of statujs(0: success, -1: failure*/
         exit(status);
      }
   }

   return success_status;
}
