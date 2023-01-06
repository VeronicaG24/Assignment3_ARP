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
#define max_num_center 80*30
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
// Data structure for storing the bitmap file
bmpfile_t *bmp;
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

//raggio cerchio
int radius = 30;


center c_old[max_num_center];
//signal handler
void sig_handler(int signo){
    if(signo==SIGINT || signo==SIGTERM){
        printf("B-received SIGINT!");
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
        perror("B-Can't unlink shared memory");
        exit(-1);
        }
        exit(0);
        
    }
    if(signal(SIGINT, sig_handler)==SIG_ERR) {
        perror("B-Can't set the signal handler for SIGINT\n");
        exit(-1);
        }
    if(signal(SIGTERM, sig_handler)==SIG_ERR) {
        perror("A-Can't set the signal handler for SIGTERM\n");
        exit(-1);
        }
}

void find_center(){
int count =0;
for (int i=0; i<599; i++) {
    for (int j=0; j<1599; j++) {
        rgb_pixel_t read = ptr[(1600*i)+j];
        if(read.alpha == pixel.alpha && read.green == pixel.green && read.blue == pixel.blue && read.red == pixel.red) {
            count += 1;
        }
        if(count == (radius*2)) {
            c.x = (j-radius)/20;
            c.y = i/20;
            break;
        }
    }

    if(count == (radius*2)) {
        break;
        }   
    }
}

void draw_distance(int num_center){
    int x = c_old[num_center-1].x;
    int y = c_old[num_center-1].y;
    //mvaddch(c.y, c.x, '0');
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

int main(int argc, char const *argv[])
{

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    //center old
    int num_center = 0;

    
    if(signal(SIGINT, sig_handler)==SIG_ERR) {
        perror("B-Can't set the signal handler for SIGINT\n");
        exit(-1);
    }
    if(signal(SIGTERM, sig_handler)==SIG_ERR) {
        perror("A-Can't set the signal handler for SIGTERM\n");
        exit(-1);
    }

    //shared memory
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
        exit(-1);
    }
    //semafori
    sem_id1 = sem_open(SEM_PATH_1, 0);
    sem_id2 = sem_open(SEM_PATH_2, 0);

    sem_wait(sem_id2);
    find_center();
    sem_post(sem_id1);
    c_old[num_center].x = c.x;
    c_old[num_center].y = c.y;
    mvaddch(c.y, c.x, '0');
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
                for(int i=1; i<=num_center;i++){
                    draw_distance(i);
                }
            }
        }

        else {
            sem_wait(sem_id2);
            find_center();
            sem_post(sem_id1);

            //printf("new: %d %d\n old:%d %d", c.x, c.y,c_old[num_center].x, c_old[num_center].y);
            //ptr = (rgb_pixel_t *)mmap(0, size, PROT_READ, MAP_SHARED,shm_fd,0);
            //if(ptr<0) {
            //    perror("B-error in mapping the shared memory:");
            //}

            //se diverso
            if(c_old[num_center].x != c.x || c_old[num_center].y != c.y) {
                //printf("different x or y");
                num_center += 1;
                c_old[num_center].x = c.x;
                c_old[num_center].y = c.y;
                draw_distance(num_center);
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
