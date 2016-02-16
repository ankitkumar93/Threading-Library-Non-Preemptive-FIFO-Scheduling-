//Created By Ankit Kumar -- akumar18@ncsu.edu
README:
	1) The mythread.h and mythreadextra.h files need to be in the directory with mythread.c file while executing the makefile in order for the library to compile as mythread.c includes both of them.
	2) The mythreadextra.h file containt mythreadinitextra functions as described in the assignment specification.
	3) I have done the extra credit for mythreadinitextra and handling no mythreadexit calls when using mythreadinitextra. When using mythreadinit only, the exit calls are needed from the user.