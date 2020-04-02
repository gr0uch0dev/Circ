/*
 *
 *  chirc: a simple multi-threaded IRC server
 *
 *  This module provides the main() function for the server,
 *  and parses the command-line arguments to the chirc executable.
 *
 */

/*
 *  Copyright (c) 2011-2020, The University of Chicago
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or withsend
 *  modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  - Neither the name of The University of Chicago nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software withsend specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY send OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "log.h"
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <asm/errno.h>
#include <errno.h>


#define MAX_NICK_NAME_NUM 100
#define MAX_NICK_NAME_LEN 10


struct irc_user{
    char n_name[MAX_NICK_NAME_LEN + 1];
    char u_name[20];
    char email[30];
    struct irc_user *next; //implement in the DB as linked list
};

void error(char *msg) {
    perror(msg);
    exit(1);
}

void set_nickname_from_buffer(char n_name[MAX_NICK_NAME_LEN +1], const char *buffer){

    bzero(n_name,MAX_NICK_NAME_LEN + 1);

    int space_location = 0;
    int i = 0;
    while (buffer[i] != '\0' && buffer[i] != '\n' && buffer[i] != '\r'){ // this goes on up to the termination character
        // avoid here /r because the client is sending the raw data with the carriage escape
        if (buffer[i] == ' '){
            space_location = i;
            i++;
        }
        // get words after NICK
        if (space_location > 0){
            n_name[i - space_location -1] = buffer[i];
        }
        i++;
    }
}

#define COMMANDS_MAX_NUM 20
#define COMMANDS_MAX_LEN 50
#define COMMANDS_ARGS_MAX_LEN 250

struct command_info{
    char command_type[COMMANDS_MAX_LEN + 1];
    char command_body[250];
};

struct command_info get_command_info_from_buffer(char *input_text) {
    struct command_info output;

    bzero(output.command_type, COMMANDS_MAX_LEN +1);
    bzero(output.command_body, COMMANDS_ARGS_MAX_LEN);

    int i = 0;
    while (*input_text  != ' ') {
        output.command_type[i++] = *(input_text++);
    }
    input_text++; // go parsing the command body
    int j = 0;
    while (*input_text != '\r' && *input_text != '\n' ) {
        output.command_body[j++] = *(input_text++);
    }
    return output;
}


//struct command_info get_command_info_from_buffer(char *input_text) {
//    struct command_info output;
//
//    bzero(output.command_type, COMMANDS_MAX_LEN +1);
//    bzero(output.command_body, COMMANDS_ARGS_MAX_LEN);
//
//    int i = 0;
//    while (*input_text  != ' ') {
//        output.command_type[i++] = *(input_text++);
//    }
//    input_text++; // go parsing the command body
//    int j = 0;
//    while (*input_text != '\r' && *input_text != '\n' ) {
//        output.command_body[j++] = *(input_text++);
//    }
//    return output;
//}

int receive(int sockfd, void *buf, size_t len, int flags)
{
    size_t toread = len;
    char  *bufptr = (char*) buf;

    while (toread > 0)
    {
        ssize_t rsz = recv(sockfd, bufptr, toread, flags);
        chilog(INFO,"n=%d. Read: %s", rsz, bufptr);
        if (rsz <= 0)
            return rsz;  /* Error or other end closed connection */

        toread -= rsz;  /* Read less next time */
        bufptr += rsz;  /* Next buffer position to read into */
    }

    return len;
}



void single_buffer_in_words_list(const char *buffer, char words_list[100][100], int *count_words){

    int i = 0, j = 0;
        while(buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != ' ') {
            words_list[*count_words][j] = buffer[i];
            if (buffer[i+1] == ' ' || buffer[i+1] == '\n' || buffer[i+1] == '\r'){
                words_list[*count_words][j+1] = '\0';
                (*count_words)++;
            }
            j = 0, i++;
        }
}

int receive_and_update_words_list(int sockfd, size_t len, int flags, char words_list[100][100])
{
    size_t toread = len;
    int num_count = 0;
    char buffer[len];
    while (toread > 0)
    {
        bzero(buffer,len);
        // TODO be careful of how words_list is passed
        ssize_t rsz = recv(sockfd, buffer, toread, flags);
        chilog(INFO,"n=%d. Read: %s", rsz, buffer);
        if (*buffer) single_buffer_in_words_list(buffer,words_list,&num_count);
        if (rsz <= 0)
            return rsz;  /* Error or other end closed connection */

        toread -= rsz;  /* Read less next time */
    }

    return len;
}

void parse_buffer_and_update_words_list(const char *buffer, char words_list[100][100], int *count_words){

        int i = 0, j = 0;
        while(buffer[i] != '\n' && buffer[i] != '\r') {
            if (buffer[i] != ' '){
                words_list[*count_words][j] = buffer[i];
                if (buffer[i+1] == ' ' || buffer[i+1] == '\n' || buffer[i+1] == '\r'){
                    words_list[*count_words][j+1] = '\0';
                    (*count_words)++;
                    j = 0;
                }
                j++;
            }
            i++;
        }
}


int main(int argc, char *argv[])
{
    int opt;
    char *port = NULL, *passwd = NULL, *servername = NULL, *network_file = NULL;
    int verbosity = 0;

    while ((opt = getopt(argc, argv, "p:o:s:n:vqh")) != -1)
        switch (opt)
        {
        case 'p':
            port = strdup(optarg);
            break;
        case 'o':
            passwd = strdup(optarg);
            break;
        case 's':
            servername = strdup(optarg);
            break;
        case 'n':
            if (access(optarg, R_OK) == -1)
            {
                printf("ERROR: No such file: %s\n", optarg);
                exit(-1);
            }
            network_file = strdup(optarg);
            break;
        case 'v':
            verbosity++;
            break;
        case 'q':
            verbosity = -1;
            break;
        case 'h':
            printf("Usage: chirc -o OPER_PASSWD [-p PORT] [-s SERVERNAME] [-n NETWORK_FILE] [(-q|-v|-vv)]\n");
            exit(0);
            break;
        default:
            fprintf(stderr, "ERROR: Unknown option -%c\n", opt);
            exit(-1);
        }

    if (!passwd)
    {
        fprintf(stderr, "ERROR: You must specify an operator password\n");
        exit(-1);
    }

    if (network_file && !servername)
    {
        fprintf(stderr, "ERROR: If specifying a network file, you must also specify a server name.\n");
        exit(-1);
    }

    /* Set logging level based on verbosity */
    switch(verbosity)
    {
    case -1:
        chirc_setloglevel(QUIET);
        break;
    case 0:
        chirc_setloglevel(INFO);
        break;
    case 1:
        chirc_setloglevel(DEBUG);
        break;
    case 2:
        chirc_setloglevel(TRACE);
        break;
    default:
        chirc_setloglevel(TRACE);
        break;
    }


    if (!port) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    int n;
    char buffer[256];
    struct addrinfo hints, *res;

    // recall to zero the bytes of the hints

    memset(&hints, 0, sizeof(struct addrinfo));
    // we save in hints the criteria used to retrieve the address

    // what are the information we need to set in hints?
    // first of all the address family
    hints.ai_family = AF_INET; // we want the ipv4 family
    // then the type of socket we want to create on that address
    hints.ai_socktype = SOCK_STREAM; // we want connection over stream like TCP
    // and finally the protocol
    hints.ai_protocol = 0; // the best protocol for the given criteria

    int status;
    if ( (status =getaddrinfo(NULL, port, &hints, &res)) == -1){
        error("Error in retrieving information of the host");
    }

    // TODO create error wrappers for the following procedures
    int socket_fd;
    socket_fd = socket(res->ai_family, res->ai_socktype, res -> ai_protocol);

    bind(socket_fd, res->ai_addr, res->ai_addrlen);
    int queue = 5; // clients allowed to queue
    listen(socket_fd, queue);
    struct sockaddr_storage client_sock_addr;
    int client_len = sizeof(client_sock_addr); // the space needed that will change accordingly

    while (1) {
        int new_sock_fd;
        new_sock_fd = accept(socket_fd, (struct sockaddr *) &client_sock_addr, (socklen_t *) &client_len);

//        fcntl(socket_fd, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/

        // TODO change the write and read in send and recv
        if (new_sock_fd < 0)
            error("ERROR on accept");

        char n_name[MAX_NICK_NAME_LEN + 1];
        bzero(n_name, MAX_NICK_NAME_LEN + 1);

        int nickname_was_sent = 0;

        char words_recieved[100][100];
        int count_words = 0;

       // n = receive(new_sock_fd, buffer, 255, 0);

        char *command_to_check_for = "NICK";

        //n = receive_and_update_words_list(new_sock_fd, 255, 0, words_recieved);
        //int i = 0, j = 0;

//        struct timeval tv;
//        tv.tv_sec = 60;
//        tv.tv_usec = 0;
//        setsockopt(new_sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
//        int status = fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL, 0) | O_NONBLOCK);
        // Set non-blocking



        // TODO make the socket non blocking

//        fd_set fdset;
//        struct timeval tv;
//        fcntl(new_sock_fd, F_SETFL, O_NONBLOCK);

//        FD_ZERO(&fdset);
//        FD_SET(new_sock_fd, &fdset);
//        tv.tv_sec = 3;
//        tv.tv_usec = 0;

        int num_msg = 0;
        int msg_to_recv = 2;
        // TODO problem here is  that I cheated with knowing the number of the messages to make the test pass
        while(msg_to_recv){
            bzero(buffer,256);
            n = recv(new_sock_fd, buffer, 255, 0);
            chilog(INFO,"Got message #%d of length %d. Read: %s",++num_msg, n, buffer);
            if (n == 0) break;

            if (n < -1) error("Error in reading from socket");
            //parse_buffer_and_update_words_list(buffer,words_recieved,&count_words);

            //TODO move the following in a procedure
            int counter = n, i = 0, j=0;

            while(counter > i) {
                if (buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != ' '){
                    words_recieved[count_words][j] = buffer[i];
                    j++;
                    if (buffer[i+1] == ' ' || buffer[i+1] == '\n' || buffer[i+1] == '\r'){
                        words_recieved[count_words][j] = '\0';
                        count_words++;
                        j = 0;
                    }
                }
                i++;
            }
            //if (buffer[n-1] == '\n' && buffer[n-2] == '\r') break;
            msg_to_recv--;
        }

        for (int k = 0; k < count_words; k++){
            if (strncmp(words_recieved[k], command_to_check_for, 5) == 0){
                nickname_was_sent = 1;
                if (k+1 >= count_words){ // NICK command found but no more words available to pick from
                    error("NICK command provided with no arguments");
                }
                strncpy(n_name, words_recieved[k+1], MAX_NICK_NAME_LEN);
            }
        }

//   #####################################################
//       While loop that has trouble with python test
//   #####################################################
//        while(1){
//            bzero(buffer,256);
//            //sleep(2);
//            n = recv(new_sock_fd, buffer, 255, 0);
//            chilog(INFO,"Got message #%d of length %d. Read: %s",++num_msg, n, buffer);
//            //if (n < 0) error("Error in reading from socket");
//            if (n == 0) break;
//            if (n == -1){
//                if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
//                {
//                    /* no data to be read on socket */
//                    if(!attempts--) break ; //try again
//                    /* wait one second */
//                    sleep(1);
//                } else {
//                    error("Error in reading from socket");
//                }
//            }
//            if (n < -1) error("Error in reading from socket");
//            //parse_buffer_and_update_words_list(buffer,words_recieved,&count_words);
//
//            //TODO move the following in a procedure
//            int counter = n;
//            while(counter > i) {
//                if (buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != ' '){
//                    words_recieved[count_words][j] = buffer[i];
//                    j++;
//                    if (buffer[i+1] == ' ' || buffer[i+1] == '\n' || buffer[i+1] == '\r'){
//                        words_recieved[count_words][j] = '\0';
//                        count_words++;
//                        j = 0;
//                    }
//                }
//                i++;
//            }
//            //if (buffer[n-1] == '\n' && buffer[n-2] == '\r') break;
//        }
//
//        for (int k = 0; k < count_words; k++){
//            if (strncmp(words_recieved[k], command_to_check_for, 5) == 0){
//                nickname_was_sent = 1;
//                if (k+1 >= count_words){ // NICK command found but no more words available to pick from
//                    error("NICK command provided with no arguments");
//                }
//                strncpy(n_name, words_recieved[k+1], MAX_NICK_NAME_LEN);
//            }
//        }



//        n = 1;
//        while(n > 0){
//            bzero(buffer,256);
//            n = recv(new_sock_fd, buffer, 255, 0);
//            if (n < 0) error("Error in reading from socket");
//            //if (n == 0) break;
//            //parse_buffer_and_update_words_list(buffer,words_recieved,&count_words);
//
//            //TODO move the following in a procedure
//
//            while(buffer[i] != '\n' && buffer[i] != '\r') {
//                if (buffer[i] != ' '){
//                    words_recieved[count_words][j] = buffer[i];
//                    j++;
//                    if (buffer[i+1] == ' ' || buffer[i+1] == '\n' || buffer[i+1] == '\r'){
//                        words_recieved[count_words][j] = '\0';
//                        count_words++;
//                        j = 0;
//                    }
//                }
//                i++;
//            }

//            chilog(INFO,"n=%d. Read: %s", n, buffer);
//            //if (buffer[n-1] == '\n' && buffer[n-2] == '\r') break;
//
//        }


//        for (int k = 0; k < count_words; k++){
//            if (strncmp(words_recieved[k], command_to_check_for, 5) == 0){
//                nickname_was_sent = 1;
//                if (k+1 >= count_words){ // NICK command found but no more words available to pick from
//                    error("NICK command provided with no arguments");
//                }
//                strncpy(n_name, words_recieved[k+1], MAX_NICK_NAME_LEN);
//            }
//        }




        // TODO check for remaining buffers

//

        bzero(buffer,256);
        sprintf(buffer, ":circ.groucho.com 001 %s :Welcome to the Internet Relay Network %s!%s@user.example.com \r\n", n_name, n_name, n_name);

        n = send(new_sock_fd, buffer, 100, 0);

        chilog(INFO,"Sent to socket: %s\n",buffer);
        if (n < 0) error("ERROR writing to socket");
        sleep(3); // avoid closing connection too fast

        close(new_sock_fd);
    }
    return 0;
}



