/*===========================================================================
-----------------------------------------------------------------------------
  	master.c
-----------------------------------------------------------------------------

AUTHOR: Written by Francesca Corrao and Veronica Gavagna.

-----------------------------------------------------------------------------

DESCRIPTION
  	The master program spawns the other processes, creates the log file, 
    and waits for the processes close to close itself.

=============================================================================*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>


/*=====================================
  Spawn processes
  RETURN:
    1 if errors while forking happed
    1 if exec failed
    else child process pid
=====================================*/
int spawn(const char * program, char * arg_list[]) {

  pid_t child_pid = fork();

  if(child_pid < 0) {
    perror("Error while forking...");
    return 1;
  }

  else if(child_pid != 0) {
    return child_pid;
  }

  else {
    if(execvp (program, arg_list) == 0);
    perror("Exec failed");
    return 1;
  }
}

/*=====================================
  Manage processes and create log file
  RETURN:
    0 when exit
=====================================*/
int main() {

  char * arg_list_A[] = { "/usr/bin/konsole", "-e", "./bin/processA", NULL };
  char * arg_list_B[] = { "/usr/bin/konsole", "-e", "./bin/processB", NULL };

  //reset log file if exists
  if(remove("./logFile.log")!=0){
    perror("Log file not deleted:");
  }
  //create new logfile
  fclose(fopen("./logFile.log", "w"));

  //generate processA and processB
  pid_t pid_procA = spawn("/usr/bin/konsole", arg_list_A);
  sleep(1);
  pid_t pid_procB = spawn("/usr/bin/konsole", arg_list_B);
  sleep(1);

  int status;
  //wait until proccessA and processB end
  waitpid(pid_procB, &status, 0);
  printf("process B terminate with status %d\n", status);
  waitpid(pid_procA, &status, 0);
  printf("process A terminate with status %d\n", status);
  
  printf ("Main program exiting with status %d\n", status);
  return 0;
}

