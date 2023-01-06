/*===========================================================================
-----------------------------------------------------------------------------
  	processA.c
-----------------------------------------------------------------------------

AUTHOR: Written by Francesca Corrao and Veronica Gavagna.

-----------------------------------------------------------------------------

DESCRIPTION
  	ProcessA manages the user interface, the movement of the circle 
    due to the pression of the arrow button on the keyboard, crates and 
    updates the shared memory, creates and updates the local bitmap and 
    if you press the button "P" it prints/saves the bitmap with the circle 
    as a ".bmp" file.

=============================================================================*/

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

//struct for center
typedef struct {
    int x, y;
} center;

// shared memory
const char *shm_name = "\bitmap";
const int size = 1600 * 600 * sizeof(rgb_pixel_t);
int shm_fd;
rgb_pixel_t *ptr;

// semaphores
sem_t *sem_id1;
sem_t *sem_id2;

// Data structure for storing the bitmap file
bmpfile_t *bmp;
// Data type for defining pixel colors (BGRA)
rgb_pixel_t pixel = {255, 0, 0, 0};
// size of bitmap
int width = 1600;
int height = 600;
int depth = 4;

//radius of the circle
int radius = 30;


/*=====================================
  Manage signals received
  INPUT:
  SIGINT or SIGTERM
    -close semaphores
    -unlink semaphores
    -unmap shared memory
  RETURN:
    null
=====================================*/
void sig_handler(int signo){
    if(signo==SIGINT || signo==SIGTERM) {
        printf("A-received SIGINT!");
        fflush(stdout);
        
        // close semaphore
        sem_close(sem_id1);
        sem_close(sem_id2);
        sem_unlink(SEM_PATH_1);
        sem_unlink(SEM_PATH_1);

        // unmmap pointer shared memory
        munmap(ptr, size);
        // close shared memory
        if (shm_unlink(shm_name) == -1)
        {
            perror("A-Can't unlink shared memory");
            exit(-1);
        }

        exit(0);
    }

    //manage errors in handling signals
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        perror("A-Can't set the signal handler for SIGINT\n");
        exit(-1);
    }
    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        perror("A-Can't set the signal handler for SIGTERM\n");
        exit(-1);
    }
}


/*=====================================
  Distroy the old bitmap
  Create a new bitmap
  Draw circle on the bitmap
  INPUT:
    -x of the center
    -y of the center
  RETURN:
    null
=====================================*/
void draw_bmp(int xc, int yc) {
    //distry old bitmap
    bmp_destroy(bmp);
    //create a new bitmap
    bmp = bmp_create(width, height, depth);

    //draw the circle
    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            // If distance is smaller, point is within the circle
            if (sqrt(x * x + y * y) < radius) {
                /*
                 * Color the pixel at the specified (x,y) position
                 * with the given pixel values
                 */
                bmp_set_pixel(bmp, xc + x, yc + y, pixel);
            }
        }
    }
}


/*=====================================
  Manage the user interface,
  the motion of the circle, and the
  print button.
  RETURN:
    0 when exit
=====================================*/
int main(int argc, char *argv[]) {

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    //manage signals
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        perror("A-Can't set the signal handler for SIGINT\n");
        exit(-1);
    }
    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        perror("A-Can't set the signal handler for SIGTERM\n");
        exit(-1);
    }

    // local bitmap
    bmp = bmp_create(width, height, depth);
    draw_bmp((circle.x)*20,(circle.y)*20);
    
    // open the shared memery
    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1){
        perror("A-error in open the shared memory:");
        exit(-1);
    }

    //set the shared memory on the right dimension
    if(ftruncate(shm_fd,size)==-1){
        perror("A-error in truncate the shared memory");
    }
    
    //pointer to reference the shared memory
    ptr= (rgb_pixel_t *)mmap(0, size,PROT_WRITE, MAP_SHARED,shm_fd,0);
    if(ptr<0){
        perror("A-error in mapping the shared memory:");
        exit(-1);
    }

    // semaphores
    sem_id1 = sem_open(SEM_PATH_1, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_id2 = sem_open(SEM_PATH_2, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_init(sem_id1, 1, 1); // initialized to 1
    sem_init(sem_id2, 1, 0); // initialized to 0

    //send first bitmap
    sem_wait(sem_id1);
    //send first position of the center
    for(int i=0; i<=599; i++){
        for (int j=0; j<=1599; j++){
            int index=(1600*i)+j;
            rgb_pixel_t * read = bmp_get_pixel(bmp,j,i);
            ptr[index].alpha=read->alpha;
            ptr[index].blue=read->blue;
            ptr[index].green=read->green;
            ptr[index].red=read->red;
        } 
    }
    sem_post(sem_id2);

    // Infinite loop
    while (TRUE) {

        // Get input in non-blocking mode
        int cmd = getch();

        // If user resizes screen, re-draw UI...
        if (cmd == KEY_RESIZE) {
            if (first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
            }
        }

        // Else, if user presses print button...
        else if (cmd == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                if (check_button_pressed(print_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Print button pressed");
                    // print bitmap
                    bmp_save(bmp, "./out/bitmap.bmp");
                    refresh();
                    sleep(1);
                    
                    for (int j = 0; j < COLS - BTN_SIZE_X - 2; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }
            }
        }

        // If input is an arrow key, move circle accordingly...
        else if (cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN) {
            
            move_circle(cmd);
            draw_circle();

            // draw circle of the bitmap
            draw_bmp((circle.x)*20, (circle.y)*20);
            
            // copy on the shared memory
            sem_wait(sem_id1);
            //send new position of the center
            for(int i=0; i<=599; i++){
                for (int j=0; j<=1599; j++){
                    int index=(1600*i)+j;
                    rgb_pixel_t * read = bmp_get_pixel(bmp,j,i);
                    ptr[index].alpha=read->alpha;
                    ptr[index].blue=read->blue;
                    ptr[index].green=read->green;
                    ptr[index].red=read->red;
                } 
            }
            sem_post(sem_id2);
        }
    }

    endwin();
    printf("closing\n");
    fflush(stdout);
    sleep(5);

    // unmmap pointer of the shared memory
    munmap(ptr, size);
    // close shared memory
    if (shm_unlink(shm_name) == -1) {
        perror("A-Can't unlink shared memory");
    }

    // close semaphores
    sem_close(sem_id1);
    sem_close(sem_id2);
    sem_unlink(SEM_PATH_1);
    sem_unlink(SEM_PATH_1);
    
    return 0;
}
