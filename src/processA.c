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
    as a ".bmp" file. It updates log file.

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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SEM_PATH_1 "/sem_w"
#define SEM_PATH_2 "/sem_r"

//struct for center coordinates
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

//mode 0:normal 1:client 2:server
int mode;

//socket variables 
int sockfd, newsockfd, portno, clilen, n;
struct sockaddr_in serv_addr, cli_addr;
struct hostent *server;

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


/*=====================================
  Close semaphores and unmap shared
  memory
  RETURN:
    value of exit
        -1 if error
        0 if ok
=====================================*/
int release_resouces(){
    int ret=0;
    // close semaphores
    if(sem_close(sem_id1)==-1){
        perror("A-Can't close semaphore 1");
        ret=-1;
    }
    if(sem_close(sem_id2)==-1){
        perror("A-Can't close semaphore 2");
        ret=-1;
    }
    if(sem_unlink(SEM_PATH_1)==-1){
        perror("A- Can't unlink semaphore 1");
        ret=-1;
    }
    if(sem_unlink(SEM_PATH_2)==-1){
        perror("A: Can't unlink semaphore 2");
        ret=-1;
    }

    // unmmap pointer shared memory
    if(munmap(ptr, size)==-1){
        perror("A-can't unmap correctly");
        ret=-1;
    }
    // close shared memory
    if (shm_unlink(shm_name) == -1) {
        perror("A-Can't unlink shared memory");
        ret=-1;
    }

    return(ret);
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
        printf("A-received SIGINT!");
        fflush(stdout);
        int ret_val;
        // close semaphore
        ret_val=release_resouces();
        exit(ret_val);
    }

    //manage errors in handling signals
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        perror("A-Can't set the signal handler for SIGINT\n");
        release_resouces();
        exit(-1);
    }
    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        perror("A-Can't set the signal handler for SIGTERM\n");
        release_resouces();
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

    //update log file
    FILE *flog;
    flog = fopen("logFile.log", "a+");
    if (flog == NULL) {
        perror("ProcessA- cannot open log file");
    }
    else {
        char * curr_time = current_time();
        fprintf(flog, "< PROCESS A > draw new circle with center (%d, %d) on the bitmap at time: %s \n", xc, yc, curr_time);
    }
    fclose(flog);
}


/*=====================================
  Manage the user interface,
  the motion of the circle, and the
  print button. Update log file.
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
        release_resouces();
        exit(-1);
    }

    // semaphores
    if((sem_id1 = sem_open(SEM_PATH_1, O_CREAT, S_IRUSR | S_IWUSR, 1))==SEM_FAILED){
        perror("A-error in opening semaphore 1");
        release_resouces();
        exit(-1);
    }
    if((sem_id2 = sem_open(SEM_PATH_2, O_CREAT, S_IRUSR | S_IWUSR, 1))==SEM_FAILED){
        perror("A-error in opening semaphore 2");
        release_resouces();
        exit(-1);
    }
    if(sem_init(sem_id1, 1, 1)==-1){
        perror("A-error in init semaphore1");
        release_resouces();
        exit(-1);
    }
    if( sem_init(sem_id2, 1, 0)==-1){
        perror("A-error in init semaphore 2");
        release_resouces();
        exit(-1);
    }

    //send first bitmap
    if(sem_wait(sem_id1)==-1){
        perror("A-wait sem1");
        release_resouces();
        exit(-1);
    }
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
    if(sem_post(sem_id2)==-1){
        perror("A-can't post sem2");
        release_resouces();
        exit(-1);
    }
    //ask user for type of process
    
    scanf("What kind of mode process run has to run ? 0 normal - 1 client - 2 server: %d", &mode);
    switch(mode){
        case 0:
            //normal execution nothing to do;
            break;
        
        case 1:
            //client
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if(sockfd <0){
                perror("error in opening socket");
            }
            //get server address by name or in other way
            char * server_name;
            struct hostent *server;
            if (server == NULL) {
                fprintf(stderr,"ERROR, no such host\n");
                exit(0);
            }
            scanf("name of the host %s", server_name);
            scanf("portnumber to use: %d", &portno);
            server= gethostbyname(server_name);
            serv_addr.sin_family=AF_INET;
            serv_addr.sin_port=htons(portno);
            bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
            //connect
            connect(sockfd, &serv_addr, sizeof(serv_addr));
            break;

        
        case 2:
            //server
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if(sockfd <0){
                perror("error in opening socket");
            }
            //get port number 
            scanf("portnumber to use: %d", &portno);
            serv_addr.sin_family=AF_INET;
            serv_addr.sin_port=htons(portno);
            serv_addr.sin_addr.s_addr=INADDR_ANY;

            //bind
            if(bind(sockfd, (struct sockadrr *)&serv_addr, sizeof(serv_addr))<0){
                perror("error in bind");
            }

            //listen
            listen(sockfd,5); 

            //accept 
            newsockfd = accept(sockfd,(struct sockadrr *)&cli_addr, sizeof(cli_addr));
            if(newsockfd <0){
                perror("error in opening socket");
            }
            break;
        
        default:
            printf("error unrecognized value inserted");
            //ask again value;
        
    }


    // Infinite loop
    while (TRUE) {

        // Get input in non-blocking mode
        int cmd = getch();
        //add cmd2 when server read from socket when client/normale = cmd
        int cmd2=cmd;
        
        if (mode ==2){
            //read from socket cmd
            read(newsockfd, &cmd2, sizeof(int));
            //convert in int the value read from the socket
        }
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

                    //update log file
                    FILE *flog;
                    flog = fopen("logFile.log", "a+");
                    if (flog == NULL) {
                        perror("ProcessA: cannot open log file");
                    }
                    else {
                        char * curr_time = current_time();
                        fprintf(flog, "< PROCESS A > print bitmap at time: %s \n", curr_time);
                    }
                    fclose(flog);
                    
                    for (int j = 0; j < COLS - BTN_SIZE_X - 2; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }
            }
        }

        // If input is an arrow key, move circle accordingly...
        else if (cmd2 == KEY_LEFT || cmd2 == KEY_RIGHT || cmd2 == KEY_UP || cmd2 == KEY_DOWN) {
            /*
            if(mode == 1){
                //send on the socket
                write(sockfd, &cmd2, sizeof(int));
            }
            */
            move_circle(cmd);
            draw_circle();

            // draw circle of the bitmap
            draw_bmp((circle.x)*20, (circle.y)*20);
            
            // copy on the shared memory
            if(sem_wait(sem_id1)==-1){
                perror("A-wait sem1");
                release_resouces();
                exit(-1);
            }
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
            if(sem_post(sem_id2)==-1){
                perror("A-can't post sem2");
                release_resouces();
                exit(-1);
            }
        }
    }

    endwin();
    printf("closing\n");
    fflush(stdout);
    sleep(5);
    int ret_val;
    ret_val=release_resouces();
    return ret_val;
}
