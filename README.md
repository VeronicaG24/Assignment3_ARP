# ARP-Assignment2
Base repository for the **second ARP assignment**.
The project provides you with a base infrastructure for the implementation of the simulated vision system through shared memory, according to the requirements specified in the PDF file of the assignment.

The two processes involved in the simulation of the vision system, namely **processA** and **processB**, are implemented as simple *ncurses windows*. The development of the inter-process communication pipeline, that is the shared memory, is left to you.

As for the first assignment, you also find a **master** process already prepared for you, responsible of spawning the entire simulation.

Additionally, I have prepared a simple program called **circle.c**, which shows you the basic functionalities of the *libbitmap* library. Please, note that the **circle.c** process must not appear in your final project. It is simply meant to be a guide for you on how to use the bitmap library, therefore you will need to properly use portions of that code in **processA** and **processB** in order to develop your solution.

## libbitmap installation setup
