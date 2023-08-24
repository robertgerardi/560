##
## Keil uVision Call Graph Generator
## Michael D'Argenio - mjdargen@ncsu.edu
## Initial: February 28, 2019
## Last Revised: April 17, 2019
##
## This program takes a default .txt callgraph output from Keil uVision,
## parses it, and transforms it into a .gv Graphviz gvedit file to
## visually display the call graph for your project.
##
## AD To Do: add arguments to control show_stack_usage, quiet output, overwrite pdf option
##
##

import re
from tkinter import *
from tkinter.filedialog import askopenfilename
from tkinter.filedialog import asksaveasfilename
import sys, getopt
	
# AD: Likely root functions: *_IRQHandler, 
# svcRtx*, osRtx*_Handler, osRtx*Thread
# main, Thread_*

def IsOSSupervisorCallName(funname):
	# RTX 5 supervisor call handlers
	p = re.compile('svcRtx\w+')
	if p.match(funname):
		print('OS SVC Handler: %s' %funname)
		return True
	return False
	
def IsOSThreadName(funname):
	# RTX 5 Threads
	p = re.compile('osRtx\w+Thread')
	if p.match(funname):
		print('OS Thread: %s' %funname)
		return True
	return False

def IsOSHandlerName(funname):
	# RTX 5 Handlers 
	p = re.compile('osRtx\w+_Handler')
	if p.match(funname):
		print('OS Thread: %s' %funname)
		return True
	return False
	
def IsIRQHandlerName(funname):
	# IRQ Handlers
	p = re.compile('\w+_IRQHandler')
	if p.match(funname):
		print('IRQ Handler: %s' %funname)
		return True
	return False

def IsAppThreadName(funname):
	# IRQ Handlers
	p = re.compile('Thread_\w+')
	if p.match(funname):
		print('App Thread: %s' %funname)
		return True
	return False

def ColorCodeNode(funname):
	if IsOSSupervisorCallName(funname):
		return ', fillcolor=gray90, style=filled'
	elif IsOSThreadName(funname):
		return ', fillcolor=orange3, style=filled'
	elif IsOSHandlerName(funname):
		return ', fillcolor=yellow3, style=filled'
	elif IsIRQHandlerName(funname):
		return ', fillcolor=yellow1, style=filled'
	elif IsAppThreadName(funname):
		return ', fillcolor=orange1, style=filled'
	elif funname == 'main':
		return ', fillcolor=yellow, style=filled'
	else:
		return ', fillcolor=white, style=filled'

def IsRootFunctionName(funname):
	if IsOSSupervisorCallName(funname):
		return True
	elif IsOSThreadName(funname):
		return True
	elif IsOSHandlerName(funname):
		return True
	elif IsIRQHandlerName(funname):
		return True
	elif IsAppThreadName(funname):
		return True
	elif funname == 'main':
		return True
	else:
		return False

inputfilename = None
outputfilename = None
show_stack_usage = False
try:
	opts, args = getopt.getopt(sys.argv[1:],"i:o:",[])
except getopt.GetoptError:
	print('arguments -i <inputfile> -o <outputfile>')
	sys.exit(2)
for opt, arg in opts:
	if opt in ("-i"):
		inputfilename = arg
	elif opt == "-o":
		outputfilename = arg
print('Input file is: ', inputfilename)
print('Output file is: ', outputfilename)

if inputfilename is None:
	## setup input file
	## input must be .txt callgraph file as output from Keil uVision
	## pop-up dialog box to select .txt file
	Tk().withdraw() # minimal window
	inputfilename = askopenfilename(filetypes = [('text files', '*.txt'),])
	# if input file not provided, exit
	if inputfilename is None:
			exit

if outputfilename is None:
	## setup output file
	## output is a .gv file created to create a visual call graph in GraphViz gvedit
	## pop-up dialog box to save .gv file
	outputfilename = asksaveasfilename(filetypes = [('Graphviz files', '*.gv'),])
	# if output file not provided, exit
	if outputfilename is None:
			exit

## open input/output files
inputfile = open(inputfilename, 'r')
outputfile = open(outputfilename, 'w')

case = 0; # used as case statement

g_Functions=[]; # global array/list of functions

# initial Graphiz declarations
outputfile.writelines('digraph mycallgraph {\n')
outputfile.writelines('\nnode [shape=box, fontname=Helvetica];\n\n')

# read in new lines of text until there are no more
# first while loop is for retrieving all necessary function names
line = inputfile.readline()
while line:
	
		# why doesn't python have switch statements??
		# case 0: initial state
		# do nothing state until you find Global Symbols line 
		if case == 0:
				if 'Global Symbols\n' in line:
						case = 1

		# case 1: function receive state
		# looks for functions, stores function name
		# prints if not a library function
		elif case == 1:
				if 'Thumb' in line:

						#left strip garbage
						line = line.lstrip()

						# use regular expressions to detect and retrieve word (caller name)
						pattern = re.compile(r"\w*")
						caller_temp = re.match(pattern, line).group(0)
						# if it begins with _ (underscore) exclude
						# this likely means that it is a library function
						# that will just clutter the call graph
						# AD: Add exclusion list test here
						if ((caller_temp != None) and (caller_temp.find('_')!=0)): # there was a match
								caller = caller_temp

								# retrieving stack size
								stack_index = line.find('Stack size')
								if (stack_index!=-1):
										stack_temp = line[stack_index:]
										if stack_temp != None: # there was a match
												stack_size = stack_temp.split('bytes',1)[0]
												stack_size = stack_size.split('e',1)[1]
												stack_size = stack_size.rstrip()
												stack_size = stack_size.lstrip()
												# if stack size is 0 and its a Handler, don't include
												if ((stack_size == "0") and (caller.find('Handler')!=-1)):
														caller = None
												# sometimes stack is zero, but it should be included
												# if the size is 0, do not add to graph
												# avoids ISRs and other functions with no body
												empty_index = line.find('bytes, Stack size')
												if (empty_index!=-1):
														empty = line[:stack_index]
														empty = empty.split('bytes,',1)[0]
														empty = empty.split(',',1)[1]
														empty = empty.rstrip()
														empty = empty.lstrip()
														if (empty == "0"):
																caller = None
												else:
														caller = None
												stack_size = "Stack Size = " + stack_size

										# retrieving max depth
										# eat 4 lines then check if max depth is listed
										line = inputfile.readline()
										line = inputfile.readline()
										line = inputfile.readline()
										line = inputfile.readline()
										if (line.find('Max Depth')!=-1):
												depth_index = line.find('Max Depth')
												if (depth_index!=-1):
														# clear out call chain info
														chain_index = line.find('Call Chain')
														max_depth = line[depth_index:chain_index-1]
														max_depth = max_depth.rstrip()

								# if there was a caller with a stack size, print
								if (caller != None):
										g_Functions.append(caller)
										outputfile.writelines('node [label="%s\n' %caller)
										if (show_stack_usage):
											if 'stack_size' in locals():
													outputfile.writelines('%s \n' %stack_size)
											if ('max_depth' in locals()) and (IsRootFunctionName(caller)):
													outputfile.writelines('%s' %max_depth)
										outputfile.writelines('"')
										# AD: pretty print root function names
										outputfile.writelines(ColorCodeNode(caller))
										outputfile.writelines(']; %s;\n' %caller)

						
		# get new line to iterate over
		# if just \n new line, eat line
		line = inputfile.readline()
		while line == '\n':
				line = inputfile.readline()

# AD: Copy g_Functions to g_Root_Functions
g_Root_Functions = list(g_Functions);

## reinitialize for drawing edges
## close and re-open input file
outputfile.writelines('\n')
inputfile.close()
inputfile = open(inputfilename, 'r')

case = 0; # used as case statement

# read in new lines of text until there are no more
# second while loop for drawings edges between functions
line = inputfile.readline()
while line:
	
		# why doesn't python have switch statements??
		# case 0: initial state
		# do nothing state until you find Global Symbols line 
		if case == 0:
				if 'Global Symbols\n' in line:
						case = 1

		# case 1: caller receive state
		# looks for caller functions, stores caller function name
		# advances to next state if caller state has callees
		elif case == 1:
				if 'Thumb'  in line:

						#left strip garbage
						line = line.lstrip()

						# use regular expressions to detect and retrieve word (caller name)
						pattern = re.compile(r"\w*")
						caller_temp = re.match(pattern, line).group(0)
						#if ((caller_temp != None) and (caller_temp.find('_')!=0)): # there was a match
						if (caller_temp in g_Functions):
								print(caller)
								caller = caller_temp 
								
				elif '[Calls]' in line:
						if (caller_temp in g_Functions):
								case = 2 # transition to callee receive state

		# case 2: callee receive state
		# looks for callee functions. draws callgraph from caller to callee
		# until there are no more callees for that particular caller
		elif case == 2: 
				if '*' in line:

						#left strip garbage
						line = line.lstrip()
						line = line.lstrip(' * ')

						# use regular expressions to detect and retrieve word (callee name)
						pattern = re.compile(r"\w*")
						callee_temp = re.match(pattern, line).group(0)
						if (callee_temp in g_Functions):
								callee = callee_temp
								outputfile.writelines('%s' %caller)
								outputfile.writelines(' -> %s;\n' %callee)
								# AD: Remove callee from g_Root_Functions
								if callee in g_Root_Functions:
									g_Root_Functions.remove(callee)
						#if ((callee_temp != None) and (callee_temp.find('_')!=0)): # there was a match
								# if it begins with _ (underscore) exclude
								# this likely means that it is a library function
								# that will just clutter the call graph
								#callee = callee_temp
								#print(caller,' -> ')
								#print(callee, ' \n')
								#print(g_Functions)
								#if (caller in g_Functions) and (callee in g_Functions):
										#outputfile.writelines('%s' %caller)
										#outputfile.writelines(' -> %s;\n' %callee)
				else:
						case = 1 # transition to caller receive state

						# duplicate here so it doesn't accidentally eat a line
						if 'Thumb' in line:

								#left strip garbage
								line = line.lstrip()

								# use regular expressions to detect and retrieve word (caller name)
								pattern = re.compile(r"\w*")
								caller_temp = re.match(pattern, line).group(0)
								if ((caller_temp != None) and (caller_temp.find('_')!=0)): # there was a match
										if (caller_temp in g_Functions):
												caller = caller_temp
				

		# get new line to iterate over
		# if just \n new line, eat line
		line = inputfile.readline()
		while line == "\n":
				line = inputfile.readline()


# AD: Filter out non-root functions
# AD: Put root functions in first rank
# AD: {rank = min; .....}
outputfile.writelines('\n{ rank = same;\n')
outputfile.writelines('; '.join(g_Root_Functions))
outputfile.writelines('\n};')

# close bracket
outputfile.writelines('\n}')

# close input/output files
inputfile.close()
outputfile.close()


