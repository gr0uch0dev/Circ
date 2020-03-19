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


void from_buffer_to_words_list(const char *buffer, char words_list[100][100], int *count_words){

    int i = 0, j = 0, num_word = 0;
    int saved_at_least_a_char = 0;

    while(buffer[i] != '\n' && buffer[i] != '\r') {
        if (buffer[i] == ' ') {
            if(saved_at_least_a_char && buffer[i+1] != ' ') num_word++; // increase the number of words saved
            j = 0; // reset the counter for saving char
        } else {
            if(!saved_at_least_a_char) saved_at_least_a_char = 1;
            words_list[num_word][j] = buffer[i];
            if (buffer[i+1] == ' ' || buffer[i+1] == '\n' || buffer[i+1] == '\r') words_list[num_word][j+1] = '\0';
            j++;
        }
        i++;
    }
    *count_words = num_word + 1;
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
    int i, socket_fd;
    socket_fd = socket(res->ai_family, res->ai_socktype, res -> ai_protocol);

    bind(socket_fd, res->ai_addr, res->ai_addrlen);
    int queue = 5; // clients allowed to queue
    listen(socket_fd, queue);
    struct sockaddr_storage client_sock_addr;
    int client_len = sizeof(client_sock_addr); // the space needed that will change accordingly

    while (1) {
        int new_sock_fd;
        new_sock_fd = accept(socket_fd, (struct sockaddr *) &client_sock_addr, (socklen_t *) &client_len);

        // TODO change the write and read in send and recv
        if (new_sock_fd < 0)
            error("ERROR on accept");

        char n_name[MAX_NICK_NAME_LEN + 1];
        bzero(n_name, MAX_NICK_NAME_LEN + 1);
        bzero(buffer,256);
        int nickname_was_sent = 0;

        chilog(INFO,"Buffer before reading: %s",buffer);
        n = read(new_sock_fd,buffer,255);
        if (n < 0) error("ERROR reading from socket");

        chilog(INFO,"Buffer after reading: %s",buffer);

        int i = 0, j = 0;
        int saved_at_least_a_char = 0;

        char words_recieved[100][100];
        int count_words = 0;
        from_buffer_to_words_list(buffer, words_recieved, &count_words);

        //TODO we need to get the remaing messages from the buffer

        // TODO revisit the following
        char *command_to_check_for = "NICK";

        for (int k = 0; k < count_words; k++){
            if (strncmp(words_recieved[k], command_to_check_for, 5) == 0){
                nickname_was_sent = 1;
                if (k+1 >= count_words){ // NICK command found but no more words available to pick from
                    error("NICK command provided with no arguments");
                }
                strncpy(n_name, words_recieved[k+1], MAX_NICK_NAME_LEN);
            }
        }

        // TODO check for remaining buffers
//        chilog(INFO, "Going into buffer cycle");
//        do{
//            bzero(buffer,256);
//            n = read(new_sock_fd,buffer,255);
//            chilog(INFO, "Buffer read: %s", buffer);
//
//        } while(n > 0);
//

        chilog(INFO, "Out of buffer cycle");


        bzero(buffer,256);
        sprintf(buffer, ":circ.groucho.com 001 %s :Welcome to the Internet Relay Network %s!%s@user.example.com \r\n", n_name, n_name, n_name);



        n = write(new_sock_fd, buffer,100);
        //n = write(newsockfd, "OOO",strlen("000"));


        chilog(INFO,"Sent to socket: %s\n",buffer);
        if (n < 0) error("ERROR writing to socket");
        sleep(3); // avoid closing connection too fast

        close(new_sock_fd);
    }

    return 0;
}



