/*==============================================================================
--------------------------------------------------------------------------------
  	processB.c
--------------------------------------------------------------------------------

AUTHOR: Written by Francesca Corrao and Veronica Gavagna.

--------------------------------------------------------------------------------

DESCRIPTION
  	ProcessB reads from the shared memory and look for the center of the circle. 
    If the center of the circle is changed, it plots the new position 
    of the center of the circle. It updates log file.

================================================================================*/

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

// max number of the center to store
#define max_num_center 80*30

// struct for the center coordinates
typedef struct{
    int x, y;
} center;
center c;

//shared memory
const char* shm_name = "\bitmap";
const int size=1600*600*sizeof(rgb_pixel_t); //1600x600x4
int shm_fd;
rgb_pixel_t *ptr;

//semaphores
sem_t * sem_id1;
sem_t * sem_id2;

// Data type for defining pixel colors (BGRA)
rgb_pixel_t pixel = {255, 0, 0, 0};

/* Instantiate bitmap, passing three parameters:
*   - width of the image (in pixels)
*   - Height of the image (in pixels)
*   - Depth of the image (1 for greyscale images, 4 for colored images)
*/
int width = 1600;
int height = 600;
int depth = 4;

//circle radius
int radius = 30;

//old center coordinates
center c_old[max_num_center];


/*=====================================
  Get current time
  RETURN:
    time and date
=====================================*/
char* current_time(){
    time_t rawtime;
    struct tm * timeinfo;
    char* timedate;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    timedate = asctime(timeinfo);
    return timedate;
}

int relese_resources(){
    int ret =0;
    //close semaphore
    if(sem_close(sem_id1)==-1){
        perror("B-Can't close semaphore 1");
        ret =-1;
    }
    if(sem_close(sem_id2)==-1){
        perror("B-Can't close semaphore 2");
        ret =-1;
    }
    if(sem_unlink(SEM_PATH_1)==-1){
        perror("B- Can't unlink semaphore 1");
        ret =-1;
    }
    if(sem_unlink(SEM_PATH_2)==-1){
        perror("B- Can't unlink semaphore 2");
        ret =-1;
    }
    
    //unmap the pointer of the shared memory
    if(munmap(ptr, size)==-1){
        perror("B-can't unmap correctly");
        ret =-1;
    }
    //close shared memory
    if(shm_unlink(shm_name)==-1) {
        perror("B-Can't unlink shared memory");
        ret =-1;
    }
    return ret;
}

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
        printf("B-received SIGINT!");
        fflush(stdout);
        int ret_val;
        ret_val=relese_resources();
        exit(ret_val);
    }

    //manage errors in handling signals
    if(signal(SIGINT, sig_handler)==SIG_ERR) {
        perror("B-Can't set the signal handler for SIGINT\n");
        relese_resources();
        exit(-1);
    }
    if(signal(SIGTERM, sig_handler)==SIG_ERR) {
        perror("A-Can't set the signal handler for SIGTERM\n");
        relese_resources();
        exit(-1);
    }
}


/*=====================================
  Find the center of the circle
  RETURN:
    null
=====================================*/
void find_center() {
    int count = 0;
    int num_rows = 0;
    int i_max = 0;
    int j_max = 0;
    int radius_max = 0;
    for (int i=0; i<=599; i++) {
        count = 0;
        for (int j=0; j<=1599; j++) {
            rgb_pixel_t read = ptr[(1600*i)+j];
            if(read.alpha == pixel.alpha && read.green == pixel.green && read.blue == pixel.blue && read.red == pixel.red) {
                count += 1;
            }
            if((count+1) == (radius*2) || (count-1) == (radius*2)) {
                num_rows += 1;
                radius_max = count/2;
                j_max = j;
                i_max = i;
                break;
            }
        }
    }
    c.x = (j_max - radius_max)/20;
    c.y = (i_max - (num_rows/2))/20;
}


/*=====================================
  Draw distance between centers
  INPUT:
    -number of center stored
  RETURN:
    null
=====================================*/
void draw_distance(int num_center) {
    
    //previous center coordinates
    int x = c_old[num_center-1].x;
    int y = c_old[num_center-1].y;
    
    //draw '0' lines to connect old center to the new one
    while(x != c_old[num_center].x) {
        if(x < c_old[num_center].x) {
            x += 1;
            mvaddch(y, x, '0');
        }
        else {
            x -= 1;
            mvaddch(y, x, '0');
        }
    }

    while(y != c_old[num_center].y) {
        if(y < c_old[num_center].y) {
            y += 1;
            mvaddch(y, x, '0');
        }
        else {
            y -= 1;
            mvaddch(y, x, '0');
        }
    }   
}


/*=====================================
  Reads from the shared memory, 
  look for the center of the circle, 
  plots the new position of the center 
  of the circle. Update log file.
  RETURN:
    0 when exit
=====================================*/
int main(int argc, char const *argv[]) {

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    //initialize number of center stored
    int num_center = 0;

    //manage signals
    if(signal(SIGINT, sig_handler)==SIG_ERR) {
        perror("B-Can't set the signal handler for SIGINT\n");
        exit(-1);
    }
    if(signal(SIGTERM, sig_handler)==SIG_ERR) {
        perror("B-Can't set the signal handler for SIGTERM\n");
        exit(-1);
    }

    //open the shared memery
    shm_fd=shm_open(shm_name, O_RDONLY, 0666);
    if(shm_fd==-1){
        perror("B-error in open the shared memory:");
        exit(-1);
    }

    //pointer to reference the shared memory
    ptr = (rgb_pixel_t *)mmap(0, size,PROT_READ, MAP_SHARED,shm_fd,0);
    if(ptr<0){
        perror("B-error in mapping the shared memory:");
        relese_resources();
        exit(-1);
    }

    //semaphores
    if((sem_id1 = sem_open(SEM_PATH_1, 0))==SEM_FAILED){
        perror("B-error in opening semaphore 1");
        relese_resources();
        exit(-1);
    }
    if((sem_id2 = sem_open(SEM_PATH_2, 0))==SEM_FAILED){
        perror("B-error in opening semaphore 1");
        relese_resources();
        exit(-1);
    }

    //get first position of the center
    if(sem_wait(sem_id2)==-1){
        perror("B-error in wait sem2");
        relese_resources();
        exit(-1);
    }
    find_center();
    if(sem_post(sem_id1)==-1){
        perror("B-error in post sem1");
        relese_resources();
        exit(-1);
    }
    c_old[num_center].x = c.x;
    c_old[num_center].y = c.y;
    mvaddch(c.y, c.x, '0');

    //update log file
    FILE *flog;
    flog = fopen("logFile.log", "a+");
    if (flog == NULL) {
        perror("ProcessB: cannot open log file");
    }
    else {
        char * curr_time = current_time();
        fprintf(flog, "< PROCESS B > center starting position (%d, %d) at time: %s \n", c.x*20, c.y*20, curr_time);
    }
    fclose(flog);

    // Infinite loop
    while (TRUE) {
        // Get input in non-blocking mode
        int cmd = getch();

        // If user resizes screen, re-draw UI and re-draw old position of the center
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
                //re-draw old position of the center
                for(int i=1; i<=num_center;i++){
                    draw_distance(i);
                }
            }
        }

        else {

            //find the new center
            if(sem_wait(sem_id2)==-1){
                perror("B-error in wait sem2");
            }
            find_center();
            if(sem_post(sem_id1)==-1){
                perror("B-error in post sem1");
                relese_resources();
                exit(-1);
            }

            //if the center is changed
            if(c_old[num_center].x != c.x || c_old[num_center].y != c.y) {
                //update the stored position of the center
                num_center += 1;
                c_old[num_center].x = c.x;
                c_old[num_center].y = c.y;
                //draw distance from the previous position to the new one
                draw_distance(num_center);

                //update log file
                FILE *flog;
                flog = fopen("logFile.log", "a+");
                if (flog == NULL) {
                    perror("ProcessB: cannot open log file");
                }
                else {
                    char * curr_time = current_time();
                    fprintf(flog, "< PROCESS B > center position is changed (%d, %d) at time: %s \n", c.x*20, c.y*20, curr_time);
                }
                fclose(flog);

            }   
            refresh();
        }
    }

    endwin();
    printf("closing\n");
    fflush(stdout);
    sleep(5);
    int ret_val;
    ret_val=relese_resources();
    return ret_val;
}
