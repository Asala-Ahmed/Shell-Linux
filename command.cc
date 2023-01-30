
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include "command.h"

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_appendFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_appendFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void handler(int sig)
{
	FILE *f;
	f=fopen("myshell.log","a");
	fprintf(f,"log\n");
	fclose(f);
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}
	signal(SIGCHLD,handler);
	if(strcmp(_simpleCommands[0]->_arguments[0],"exit")==0)
	{
		exit(0);
	}
	// Print contents of Command data structure
	print();

	// Add execution here
	int pid;
	int defaultin = dup( 0 ); // Default file Descriptor for stdin
	int defaultout = dup( 1 ); // Default file Descriptor for stdout
	int defaulterr = dup( 2 ); // Default file Descriptor for stderr
	int fdpipe[2];
	if ( pipe(fdpipe) == -1) {
			perror( "pipe");
			exit( 2 );
		}
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		
		if(strcmp(_simpleCommands[i]->_arguments[0],"cd")==0)
		{
			chdir(_simpleCommands[i]->_arguments[1]);
		}
		if(i==0)
		{
			if(_inputFile == 0){
				dup2( defaultin, 0 );
			}
			else{
				int file= open(_inputFile , O_RDONLY | O_CREAT , 0777);
				if ( file < 0 ) {
				perror( "creat inputfile" );
				exit( 2 );}
				dup2( file, 0 );
				close( file );
			}
		}
		else
		{
			dup2( fdpipe[0], 0);
		}
		if(i == (_numberOfSimpleCommands-1))
		{
			if(_outFile == 0 && _appendFile == 0){
				dup2( defaultout ,1);
			}
			else if ( _outFile != 0 )
			{
				int file1= open(_outFile ,O_RDONLY | O_WRONLY |O_TRUNC | O_CREAT , 0777);
				if ( file1 < 0 ) {
				perror( "creat outfile" );
				exit( 2 );}
				dup2( file1, 1 );
				close( file1 );
			}
			else{
				int file1= open(_appendFile , O_WRONLY | O_APPEND | O_CREAT, 0777);
				if ( file1 < 0 ) {
				perror( "creat outfile" );
				exit( 2 );}
				dup2( file1, 1 );
				close( file1 );
			}
		}
		else
		{	
			dup2( fdpipe[ 1 ], 1 );
		}
		dup2( defaulterr, 2 );
		pid=fork();
		if ( pid == -1 ) {
			perror( "fork\n");				
			exit( 2 );
		}
		if (pid == 0) {
		close(fdpipe[0]);
		close(fdpipe[1]);
		close( defaultin );
		close( defaultout );
		close( defaulterr );		
		execvp(_simpleCommands[i]->_arguments[0],_simpleCommands[i]->_arguments);
	}
}
	dup2( defaultin, 0 );
	dup2( defaultout, 1 );
	dup2( defaulterr, 2 );
	close(fdpipe[0]);
	close(fdpipe[1]);
	close( defaultin );
	close( defaultout );
	close( defaulterr );
	if (_background == 0){
		waitpid( pid, 0, 0 );
	}
	clear();
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

int 
main()
{
	Command::_currentCommand.prompt();
	signal(SIGINT,SIG_IGN);
	yyparse();
	return 0;
}

