#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 *  there's a sender and a reciever
 *  child is sending
 * 
 */


#define pp(...) printf(__VA_ARGS__)
#define epp(...) fprintf(stderr, __VA_ARGS__)

#define mssleep(ms) usleep( ms*1000 )

#define IPC_BUFF_SIZE 32

int SEND_FD;
int RECV_FD;

pid_t create_child()
{
    pid_t id = fork();
    if (id<0){
        //print details of the error i guess?
        perror("fork");
        //end program
        exit(1); 
    }
    return id;

}


void prepare_fd()
{
    int pipe_fds[2];
    int status;
    //gives us the reading([0]) and the transmitting([1]) sides of the pipe storing them in the array
    status = pipe(pipe_fds);
    // error has occured
    if ( status != 0 )
    {
        //print details?
        perror("pipe");
        exit(1);
    }

    RECV_FD = pipe_fds[0];
    SEND_FD = pipe_fds[1];

}

void ipc_send(const char *msg)
{
    ssize_t msg_size, written_bytes;
    // plus one so we also send the null byte which will be used as an indication of the end of the msg
    msg_size = strlen(msg) + 1; 
    written_bytes = write(SEND_FD, msg, msg_size);
    if (written_bytes<msg_size){
        epp("warning, ipc_send() should've written %ld bytes, instead it has written only %ld bytes!\n",
            msg_size,
            written_bytes);
            perror("write");
            epp("\n");

        if (written_bytes<0){
            epp("error, ipc_send() failed to write\n");
            perror("write");
            epp("\n");
            close(SEND_FD);
            exit(1);
        }
    }
}



void send_loop()
{

    mssleep(200);
    pp("child sending \"hello!\"\n");
    mssleep(200);

    ipc_send("hello");

    mssleep(500);
    pp("child sending \"world!\"\n");
    mssleep(200);

    ipc_send("world!");

    mssleep(500);
    pp("child sending \"END\"\n");
    ipc_send("END");

}

int end_condition(const char *buff)
{

   if(strcmp(buff, "END") == 0)
       return 1;
   return 0;
}

void recv_loop()
{
    char buff[IPC_BUFF_SIZE] = "";
    char *current_ptr = buff;
    int end = 0;
    ssize_t bytes_read = 0;
    ssize_t current_read;

    pp("parent waiting..\n");

    while ( !end ){
        mssleep(100);
        while( bytes_read < IPC_BUFF_SIZE ){
            if ( (current_read = read(RECV_FD, current_ptr, 1)) >= 0 ){
                bytes_read += current_read;
            }
            else{
                epp("error, failed to read in recv_loop()!\n");
                perror("read");
                epp("\n");
            }

            if ( (current_read > 0) && !(*(current_ptr++)) ){
                printf("Parent recieved %ld bytes: \"%s\"\n", bytes_read, buff);
                end = end_condition(buff);
                current_ptr = buff;
                *current_ptr = 0;
                bytes_read = 0;
                break;
            }
        }
        if (bytes_read >= IPC_BUFF_SIZE){
            epp("error, buffer overflow\n");
            close(RECV_FD);
            exit(1);
        }

    }

}

void dofork()
{

    prepare_fd();
    pid_t id = create_child();
    printf("recieved id: %d, recv_fd: %d, send_fd: %d\n", id, RECV_FD, SEND_FD);

    // parent condition
    if ( id ){ 
        //the parent is recieving so we close the sending file fd
        // Why? so the EOF flag is sent when the file gets closed by the other side
        close(SEND_FD);
        SEND_FD = -1;
        recv_loop();   

    }
    //child
    else{
        //the child is sending so we close the recieving file fd
        close(RECV_FD);
        RECV_FD = -1;
        send_loop();

    }
   
    
    printf("%s exiting\n", ((id)?"parent":"child"));

    if( id )
        mssleep(1000);
    else 
        close(STDOUT_FILENO);

}


int main(int argc, const char** argv)
{
    pp("start\n");

    dofork();

    pp("end\n");

}

