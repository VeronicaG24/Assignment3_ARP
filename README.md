# ARP-Assignment3
Repository for the **third ARP assignment**.
The project provides you with a base infrastructure for the implementation of the simulated vision system through shared memory, according to the requirements specified in the PDF file of the assignment.

The two processes involved in the simulation of the vision system, namely **processA** and **processB**, are implemented as simple *ncurses windows*. 

The project provides the basic functionalities for the **processA** and **processB**, both of which are implemented through the *ncurses library* as simple GUIs. In particular, the repository is organized as follows:
- The `src` folder contains the source code for the Master, ProcessA and ProcessB processes.
- The `include` folder contains all the data structures and methods used within the ncurses framework to build the two GUIs. 
- The `bin` folder is where the executable files are expected to be after compilation.
- The `out` folder is where the printed bitmap (.bmp) will be saved.
- The `compile.sh` and `run.sh` to copile and run the project.
- The `install.md` with the instruction for installing the necessary for running the code.
- The `logFile.log` file cointains what is hapening during the execution.
- The `userGuide.txt` file contains the instruction of how to use the interface.

## How to run
To run the program it is necessary to download the repository:
```console
git clone https://github.com/VeronicaG24/Assignment3_ARP.git
```

If there is no `bin` folder and/or `out` folder in the local repository, create them.

Then, move into the folder and compile the code using:
```console
bash ./compile.sh
```
And run it:
```console
bash ./run.sh
```

## Description of the code
The code is divided into 4 processes: ProcessA, ProcessB, and Master. In each of the process, signals are manage through signal handler.

The log file is updated each time occurs one of the following event:
- from ProcessA: button "P" is pressed and the bitmap is saved as .bmp file
- from ProcessA: new circle is written on the bitmap
- from ProcessB: each time the center of the circle changed

### Master
The master program spawns the other processes, and waits until the two processes close to exit. It creates the log file.

### ProcessA
ProcessA manages the chossing of modality:
- 0: default mode
	the movement of the circle due to the pression of the arrows buttons on the keyboard, update the shared memory and if you press the button "P" it prints/saves the bitmap with the circle as a ".bmp" file.
- 1: client mode
	the IP address and the port number for the connection needs to be insert, and then the movement of the circle due to the pression of the arrows buttons on the keyboard, updates the shared memory and if you press the button "P" it prints/saves the bitmap with the circle as a ".bmp" file. All the commands related to the arrows buttons are sent through socket to the server.
- 2: server mode
	the port number is asked, and then waits until the connection with a client is established. Then, on the two windows will be displayed what the client is doing: the green cross is command by the client so the movement cannot be manage using arrows buttons. This mode also update the shared memory and if you press the button "P" it prints/saves the bitmap with the circle as a ".bmp" file.
	
To change the modality is necessary to press "Ctrl + c", and then choose if change the mode or close the window.

### ProcessB
ProcessB reads from the shared memory and look for the center. If the center of the circle is changed, it plots the new position of the center.



