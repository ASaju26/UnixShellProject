// The MIT License (MIT)
// 
// Copyright (c) 2016 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports four arguments
void freearray(char ** token, int token_count);
int main()
{

  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );
  char ** history = malloc(15*sizeof(char*));
  pid_t * historyPid = malloc(15*sizeof(char*)); 
  char input[3];
  int historyCount = 0;

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      token[i] = NULL;
    }

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr = NULL;
    //checking to see if user is attempting to access history commands 
    if(command_string[0]=='!')
    {
      //user input as string then cast to int to use as index value for arrays of commands
      strcpy(input,&command_string[1]);
      
      if(atoi(input)+1<=historyCount)
      {
        //if valid history command, accesses command stored that user wants
        //this command goes into command string where it's is checked as if the user physically entered it
      strcpy(command_string, history[atoi(input)]);
      }
      else
      {
        command_string[0]='\0';
        printf("Command not in history\n");
      }
    }                                        
                                                           
    char *working_string  = strdup( command_string );                

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this for loop and replace with your shell functionality

    int token_index  = 0;
    // for( token_index = 0; token_index < token_count; token_index ++ ) 
    // {
    //   printf("token[%d] = %s\n", token_index, token[token_index] );  
    // }
    if(token[token_index]!= NULL)
    {
      //makes sure we don't exceed allocated space for history array
      if(historyCount<15)
      {
        //both allocates and copies the string to the array index,
        //easier than malloc and strcpy but either route works
        history[historyCount] = strdup(command_string);
        historyCount++;
      }
      else
      {
        for(int i=0;i<historyCount-1;i++)
        {
          //deletes the original value of history before we overwrite it when shifting indexes up
          free(history[i]);
          //shifts the history commands array up to ensure 15 most recent commands only are kept
          history[i]=strdup(history[i+1]);
          //shifts the array of pids as well for earlier values since both are in separate arrays
          historyPid[i]=historyPid[i+1];
        }
        //shifts command string into the last slot
        //pid does not have to be shifted here because that is handled as we execute each command
        history[14] = strdup(command_string);
      }
    }
    if(token[token_index]==NULL)//checks to see if user input is empty so that it can quietly accept another line of input
    {}// the way this works is that it nothing else in the loop executes and it restarts the while loop
    
    else if((strcmp(token[token_index],"exit")==0)||(strcmp(token[token_index], "quit"))==0)
    // compares entered token with exit or quit so that we know if either of those commands are enterd
    {
      freearray(token,token_count);
      free( head_ptr );
      free( command_string );
      free (argument_ptr);
      for( int i = 0; i < historyCount; i++ )
      {
        if (history[i] != NULL)
        {
          free(history[i]);
        }
      }
      free(historyPid);
      free(history);

      exit(0); 
    }
    //checks if user entered cd
    else if((strcmp(token[token_index],"cd")==0))
    {
      // current directory is now changed to the 2nd input string if it exists
      if(token[token_index+1]==NULL)
      {
        printf("cd : Please enter the directory you would like to switch to\n");
        continue;
      }
      int error = chdir(token[token_index+1]);
      historyPid[historyCount-1] = -1;
      // the goal here is to make the user aware if cd fails by printing a warning that the folder does not exist
      if(error != 0)
      {
        printf("cd: %s: No such file or directory\n", token[token_index+1]);
      }
      freearray(token,token_count);
    }
    //checks if ls was entered
    else if((strcmp(token[token_index],"ls")==0))
    {
      // running fork and exec to create a new process that becomes ls with its different parameters
      pid_t pid = fork( );
      if( pid == 0 )
      {
        int ret = execl( "/bin/ls", token[token_index], token[1], token[2], token[3],token[4], token[5], token[6], token[7],
        token[8],token[9],token[10],NULL);
        //error message if execl does not go as expected
        if( ret == -1 )
        {
          perror("execl failed: ");
        }
      }
      else 
      {
        // waits until the ls process that we just made finishes running before continuing running the program
        int status;
        historyPid[historyCount-1] = pid;
        wait( & status );
      }
      freearray(token,token_count);
    }
    //checks if history was entered as command to then decide which type of history to display
    else if((strcmp(token[token_index], "history")==0))
    {
      historyPid[historyCount-1] = -1;
      //checks whether second token is present to avoid segfault by directly comparing
      if(token[token_index+1]!=NULL)
      {
        //double checks to make sure second token is actually -p and not another value
        if(strcmp("-p",token[token_index+1])==0)
        {
          for(int k=0;k<historyCount;k++)
          {
            printf("%d. %s (pid:%d)\n", k,history[k],historyPid[k]);
          }
        }
        //checks for case where user enters a parameter for history other than -p
        else
        {
          printf("Invalid parameter for history command");
        }
        freearray(token,token_count);
      }
      //if second token is null then prints normal format
      else
      {
        for(int k=0;k<historyCount;k++)
        {
          printf("%d. %s", k,history[k]);
        }
      }
    }
    else
    {
      // running fork and exec to create a new process that becomes /bin commands with its different parameters
      pid_t pid = fork( );
      if( pid == 0 )
      {
        //looks for passed in command in system files
        int ret= execvp(token[token_index],&token[0]);
         if( ret == -1 )
        {
         // this is when the command either does not exist or could not be found or the user intered invalid input
          printf("%s: Command not found.\n",token[token_index]);
          freearray(token,token_count);
          free(head_ptr);
          free(argument_ptr);
          break;
        }
      }
      else 
      {
        //waits till the child process finishes running
        int status;
        historyPid[historyCount-1]=pid;
        wait( & status );

      }
    }
    //frees all the necessary memory that is repeatedly allocated
    // for( int i = 0; i <MAX_NUM_ARGUMENTS; i++ )
    // {
    //   if( token[i] != NULL )
    //   {
    //     free( token[i] );
    //   }
    // }
    free( head_ptr );
  }
  //frees the remaining memory that was originally allocated at the start
  free( command_string );
  for( int i = 0; i < historyCount; i++ )
  {
    if (history[i] != NULL)
    {
      free(history[i]);
    }
  }
  free(historyPid);
  free(history);

  return 0;
  // e2520ca2-76f3-90d6-0242ac120003
}


void freearray(char ** token, int token_count)
{
  for( int i = 0; i <=token_count; i++ )
  {
            if( token[i] != NULL )
            { 
              free( token[i] );
            }
  }
}