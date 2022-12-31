#include "./../include/processB_utilities.h"
#include <bmpfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>

#define SEM_PATH_1 "/sem_w"
#define SEM_PATH_2 "/sem_r"

typedef struct{
    int x, y;
}center;
//shared memory
const char* shm_name = "\bitmap";
const int size=sizeof(int); //1600x600x3
int shm_fd;
void *ptr;
//semaphores
sem_t * sem_id1;
sem_t * sem_id2;
//signal handler
void sig_handler(int signo){
    if(signo==SIGINT){
        printf("B-received SIGINT!");
        fflush(stdout);
        sleep(5);
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
        perror("B-Can't unlink shared memory");
        }
        if(signal(SIGINT, sig_handler)==SIG_ERR) {
        perror("B-Can't set the signal handler for SIGINT\n");
        exit(-1);
        }
    }
}

int main(int argc, char const *argv[])
{
    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;
    center c;
    c.y=LINES/2;
    c.x=COLS/2;
    // Initialize UI
    init_console_ui();
    if(signal(SIGINT, sig_handler)==SIG_ERR) {
        perror("B-Can't set the signal handler for SIGINT\n");
        exit(-1);
    }
    //creare bitmap locale

    //old center

    //shared memory
    //open the shared memery
    shm_fd=shm_open(shm_name, O_RDONLY, 0666);
    if(shm_fd==-1){
        perror("B-error in open the shared memory:");
    }
    
    //pointer to reference the shared memory
    ptr= mmap(0, size,PROT_READ, MAP_SHARED,shm_fd,0);
    if(ptr<0){
        perror("B-error in mapping the shared memory:");
    }

    //semafori
    sem_id1 = sem_open(SEM_PATH_1, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_id2 = sem_open(SEM_PATH_2, O_CREAT, S_IRUSR | S_IWUSR, 1);
    // Infinite loop
    while (TRUE) {

        // Get input in non-blocking mode
        int cmd = getch();
        
        //copia da shared memory a posizione locale

        // If user resizes screen, re-draw UI...
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
            }
        }

        else {
            sem_wait(sem_id2);
            //controllare centro nuova bitmap
            int x=atoi(ptr);
            ptr += sizeof(int);
            int y=atoi(ptr);
            sem_post(sem_id1);
            //printf("%d %d", c.x, c.y);
            ptr= mmap(0, size,PROT_READ, MAP_SHARED,shm_fd,0);
            if(ptr<0){
                perror("B-error in mapping the shared memory:");
            }
            //se diverso
            if(x!=c.x || y != c.y){
                //plotta nuovo punto e distanza dal vecchio
                //dist_x=x-c_old.x;
                //dist_y=y-c_old.y;
                //plot dist
                mvaddch(y, x, '0');
                //update c
                c.x=x;
                c.y=y;
            }   
            refresh();
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
