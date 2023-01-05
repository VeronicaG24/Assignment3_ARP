# ARP-Hoist-Assignment
Base repository for the **second ARP assignment**.
The project provides you with a base infrastructure for the implementation of the simulated vision system through shared memory, according to the requirements specified in the PDF file of the assignment.

The two processes involved in the simulation of the vision system, namely **processA** and **processB**, are implemented as simple *ncurses windows*. 

The project provides the basic functionalities for the **processA** and **processB**, both of which are implemented through the *ncurses library* as simple GUIs. In particular, the repository is organized as follows:
- The `src` folder contains the source code for the Command console, Inspection console and Master, MotorX, MotorZ, World and watchdog processes.
- The `include` folder contains all the data structures and methods used within the ncurses framework to build the two GUIs. 
- The `bin` folder is where the executable files are expected to be after compilation.
- The `out` folder
- The `compile.sh` and `run.sh` to copile and run the project.
- The `install.md` with the instruction for installing the necessary for running the code.
- The `logFile.log` file cointains what is hapening during the execution.

## How to run
To run the program it is necessary to download the repository:
```console
git clone https://github.com/VeronicaG24/Assignment2_ARP.git
```

If there is no `bin` folder in the local repository, create one.

Then, move into the folder and compile the code using:
```console
bash ./compile.sh
```
And run it:
```console
bash ./run.sh
```

## Description of the code
The code is divided into 4 processes: ProcessA, ProcessB, and Master. In each of the process, signal are manage through signal handler.

### Master
The master program spawns the other processes, and waits until one of the process closes to kill all the other process.

### ProcessA
ProcessA manage the movement of the circle due to the pression of the arrow button, update the shared memory and if you press the button "P" it prints/saves the bitmap with the circle as a ".bmp" file.

### ProcessB
ProcessB reads from the shared memory and update its local bitmap. If the center of the circle is changed, it plots the new position of the circle.


//CIRCLE non ci deve essere alla fine!! 

### Circle
Circle is a simple example of a program which uses the *libbitmap* library. It generates a 100x100 colored `.bmp` file with user-defined name, depicting a blue circle of given radius. When you execute it, pass the two arguments (file name and radius value) along. Execution example: ```./bin/circle out/test.bmp 20```.

### Watchdog 
The Warchdog process checks the inactivity of the program through the log file. It continuosly check the number of bytes written on the log file and if it does not chenge for more than 60 second, it kills all the processes including itself.

The log file is updated each time occurs one of the following event:
- from MotorX and MotorZ: its position changes
- from World: the coordinates of the hoist are changed
- from Command Console: each time a button is pressed
- from Inspection Console: each time the stop button or the reset button is pressed
- from Watchdog: when an inactivity is detected


