# C-Shell
Specification for Shell Program
In this assignment you are to implement a basic shell. A shell is a command line interpreter that accepts input from the user and executes programs on behalf of the user based on the commands that the user inputs. The line that the user enters commands on is referred to as the command line. The shell repeatedly prints a prompt on the command  line, waits for the user to enter commands and executes programs. You are to write a simple Unix-like shell in the C programming language that has these features:
* If your name is xyz then the prompt should be the string xyz>
* Your shell must support I/O redirection. This includes supporting both input and output redirection in the same command i.e., you should be able to support the following: sort < f1.txt > f2.txt. However, you can assume that < will appear at most once in the command and that > will appear at most once in the command.
* Your shell must support multiple pipes.
* You do not have to support a command that has both I/O redirection and pipes.
* A built-in command is a command that changes the state of the shell or requests information about the shell’s state. You should support the following two built-in commands:
* exit: This command is used to terminate the shell.
	* history: By default this command will display the last ten commands issued. If there are fewer then 10 commands then display all previously issued commands.
	* When the user types Ctrl-C the shell should terminate.
