#include "./../include/processA_utilities.h"
#include <bmpfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>

#define SEM_PATH_1 "/sem_w"
#define SEM_PATH_2 "/sem_r"

typedef struct{
    int x, y;
}center;
//shared memory
const char* shm_name = "\bitmap";
const int size=sizeof(center);
int shm_fd;
int *ptr;
//semaphores
sem_t * sem_id1;
sem_t * sem_id2;
//signal handler
void sig_handler(int signo){
    if(signo==SIGINT || signo==SIGTERM){
        printf("A-received SIGINT!");
        fflush(stdout);
        //close semaphore
        sem_close(sem_id1);
        sem_close(sem_id2);
        sem_unlink(SEM_PATH_1);
        sem_unlink(SEM_PATH_1);
        //unmmap pointer
        //to unmap the pointer
        munmap(ptr,size);
        //close shared memory
        if(shm_unlink(shm_name)==-1){
        perror("A-Can't unlink shared memory");
        exit(-1);
        }

        exit(0);
    }
    if(signal(SIGINT, sig_handler)==SIG_ERR) {
        perror("A-Can't set the signal handler for SIGINT\n");
        exit(-1);
    }
    if(signal(SIGTERM, sig_handler)==SIG_ERR) {
        perror("A-Can't set the signal handler for SIGTERM\n");
        exit(-1);
    }
}   

int main(int argc, char *argv[])
{
    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;
    center c;
    // Initialize UI
    init_console_ui();

    if(signal(SIGINT, sig_handler)==SIG_ERR) {
        perror("A-Can't set the signal handler for SIGINT\n");
        exit(-1);
    }
    if(signal(SIGTERM, sig_handler)==SIG_ERR) {
        perror("A-Can't set the signal handler for SIGTERM\n");
        exit(-1);
    }
    //bitmap locale

    //shared memory
    //open the shared memery
    shm_fd=shm_open(shm_name, O_CREAT|O_RDWR, 0666);
    if(shm_fd==-1){
        perror("A-error in open the shared memory:");
        exit(-1);
    }
    
    //set the shared memory on the right dimension
    if(ftruncate(shm_fd,size)==-1){
        perror("A-error in truncate the shared memory");
    }
    
    //pointer to reference the shared memory
    ptr= (int *)mmap(0, size,PROT_WRITE, MAP_SHARED,shm_fd,0);
    if(ptr<0){
        perror("A-error in mapping the shared memory:");
        exit(-1);
    }


    
    /*
    //to close the shared memory 
    if(shm_unlink(shm_fd)==-1){
        perror("A-Can't unlink shared memory");
    };*/
    
    
    //semaforo
    sem_id1 = sem_open(SEM_PATH_1, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_id2 = sem_open(SEM_PATH_2, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_init(sem_id1, 1, 1); //initialized to 1
    sem_init(sem_id2, 1, 0); //initialized to 0

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
            c.x=get_x();
            c.y=get_y();
            sem_wait(sem_id1);
            //send new position of the center
            ptr[0]=circle.x;
            //ptr += sizeof(int);
            ptr[1]=circle.y;
            sem_post(sem_id2);
            ptr= (int *)mmap(0, size,PROT_WRITE, MAP_SHARED,shm_fd,0);
            if(ptr<0){
                perror("A-error in mapping the shared memory:");
            }
            //sleep(1);
            //printf("%d, %d", get_x(), get_y());
            //fflush(stdout);
            //cancella vecchia bitmap
            //disegna nuovo cerchio con centro in posizione monitor
            //copia in shared memory
        }
    }
    
    endwin();
    printf("closing\n");
    fflush(stdout);
    sleep(5);
    //unmmap pointer
    munmap(ptr,size);
    //close shared memory
    if(shm_unlink(shm_name)==-1){
        perror("A-Can't unlink shared memory");
    }
    //close semaphore
    sem_close(sem_id1);
    sem_close(sem_id2);
    sem_unlink(SEM_PATH_1);
    sem_unlink(SEM_PATH_1);
    return 0;
}
