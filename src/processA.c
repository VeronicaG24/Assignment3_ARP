#include "./../include/processA_utilities.h"
#include <bmpfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    //bitmap locale

    //shared memory
    const char* shm_name = "\bitmap";
    const int size=sizeof(int);
    int shm_fd;
    void *ptr;
    //open the shared memery
    shm_fd=shm_open(shm_name, O_CREAT|O_RDWR, 0666);
    if(shm_fd==-1){
        perror("A-error in open the shared memory:");
    }
    
    //set the shared memory on the right dimension
    if(ftruncate(shm_fd,size)==-1){
        perror("A-error in truncate the shared memory");
    }
    /*
    //pointer to reference the shared memory
    ptr= mmap(0, size,PROT_WRITE, MAP_SHARED,shm_fd,0);
    if(ptr<0){
        perror("A-error in mapping the shared memory:")
    }

    //to unmap the pointer
    mummap(ptr,size);

    //to close the shared memory 
    if(shm_unlink(shm_fd)==-1){
        perror("A-Can't unlink shared memory");
    }; 
    */
    
    //semaforo


    // Infinite loop
    while (TRUE)
    {
        // Get input in non-blocking mode
        int cmd = getch();

        // If user resizes screen, re-draw UI...
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
            }
        }

        // Else, if user presses print button...
        else if(cmd == KEY_MOUSE) {
            if(getmouse(&event) == OK) {
                if(check_button_pressed(print_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Print button pressed");
                    //stampare bitmap
                    refresh();
                    sleep(1);
                    for(int j = 0; j < COLS - BTN_SIZE_X - 2; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }
            }
        }

        // If input is an arrow key, move circle accordingly...
        else if(cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN) {
            move_circle(cmd);
            draw_circle();
            //cancella vecchia bitmap
            //disegna nuovo cerchio con centro in posizione monitor
            //copia in shared memory
        }
    }
    
    endwin();
    return 0;
}
