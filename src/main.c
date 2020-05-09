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

#include <interfaces/utils.h>
#include <reply.h>
#include <interfaces/user.h>


#define MAX_NICK_NAME_NUM 100
#define MAX_NICK_NAME_LEN 10
#define MAX_NUM_OF_PARAMS_FOR_CMD 15


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

// to export these
void parse_msg_for_cmd_and_args(const char *input_baffer, char cmd_and_args[100][100]);
void process_nick_cmd(int new_sock_fd, User *user_db, char *n_name);
void process_the_command(int socket, User *user_db, char cmd_and_args[100][100]);


void send_message_to_client(char *buffer, int socket_fd){
    int n;
    n = send(socket_fd, buffer, 100, 0);
    chilog(INFO,"Sent to socket: %s\n",buffer);
    if (n < 0) error("ERROR writing to socket");
}

void send_greetings(int socket_fd, const char* n_name,){
    char buffer[256];
    bzero(buffer,256);
    sprintf(buffer, ":circ.groucho.com %s %s :Welcome to the Internet Relay Network %s!%s@user.example.com \r\n", RPL_WELCOME, n_name, n_name, n_name);
    send_message_to_client(buffer, socket_fd);
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

    User *p_user_head = (User *) malloc(sizeof(User));
    bzero(p_user_head, sizeof(User));

    while (1) {
        int new_sock_fd;
        new_sock_fd = accept(socket_fd, (struct sockaddr *) &client_sock_addr, (socklen_t *) &client_len);

       // fcntl(new_sock_fd, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/

        // TODO change the write and read in send and recv
        if (new_sock_fd < 0)
            error("ERROR on accept");

        char n_name[MAX_NICK_NAME_LEN + 1];
        bzero(n_name, MAX_NICK_NAME_LEN + 1);

        int nickname_was_sent = 0;

        char words_received[100][100];
        char cmd_and_args_array[1+MAX_NUM_OF_PARAMS_FOR_CMD][100]; // the first element stores the command, while the others the arguments
        int count_words = 0;
        int count_commands = 0;
       // n = receive(new_sock_fd, buffer, 255, 0);


        int num_msg = 0;
        char current_read_char = 0;
        char last_read_char = 0;

        char *buffer_with_cmd_and_args = (char *) malloc(512);
        int num_chars_got = 0;
        while(1){
            // msg delimeted by CRLF
            bzero(buffer,256);
            n = recv(new_sock_fd, buffer, 255, 0);
            chilog(INFO,"Got message #%d of length %d. Read: %s",++num_msg, n, buffer);
            //if (n < 0) error("Error in reading from socket");
            if (n == 0) break;
            if (n == -1){
                perror("Error reading from socket");
            }

            // the logic is: get all the words up to \r\n. As soon as CRLF is found execute the command!

            // command1: [par1, par2] 3 levels of depth
            // command2: [par1, par2]
            for(int i = 0; i < n; i++){
                current_read_char = buffer[i];
                if (last_read_char == '\r' && current_read_char == '\n'){
                    // got end of the message
                    // all the words up to now are parts of a COMMAND + [Args]
                    parse_msg_for_cmd_and_args(buffer_with_cmd_and_args, cmd_and_args_array);
                    process_the_command(new_sock_fd, p_user_head, cmd_and_args_array);
                    //clean_the_command_buffer;
                    bzero(buffer_with_cmd_and_args, 1000);
                    num_chars_got = 0; //refresh the counter
                }
                if (current_read_char!='\r' && current_read_char!='\n'){
                    *(buffer_with_cmd_and_args + num_chars_got++) = current_read_char;
                    // save the char at memory area addressed. Pointer displaced by i units (here one byte per unit).
                }
                last_read_char = current_read_char;
            }


        }

        sleep(3); // avoid closing connection too fast

        close(new_sock_fd);
    }
    return 0;
}



void parse_msg_for_cmd_and_args(const char *input_baffer, char cmd_and_args[100][100]){
    if (*input_baffer == 0) chilog(INFO,"Handle this problem. We just got CRLF");

    int index_word = 0, i = 0, j = 0;
    char last_read_char = 0;
    char processed_char;
    while(*(input_baffer + i) != 0){
        processed_char = *(input_baffer + i);
        if (processed_char == ' '){ //space in the string buffer
            if (last_read_char != ' '){
                index_word++; //increase the word index only when we encounter the first space
                j = 0; //reset the j counter used for placing the chars in cmd_and_args[(previous index_word)]
            }
            // if at next iteration an other space is found, it is skipped (index word is not changed)
        } else{
            cmd_and_args[index_word][j++] = processed_char;
        }
        i++;
        last_read_char = processed_char;
    }

}

void process_the_command(int socket, User *user_db, char cmd_and_args[100][100]){

    if (strncmp(cmd_and_args[0], "NICK", 5) == 0){
        process_nick_cmd(socket, user_db, cmd_and_args[1]);
    } else{
        chilog(INFO, "command yet to be implemented");
    }

}

void process_nick_cmd(int new_sock_fd, User *user_db, char *n_name){

    send_greetings(new_sock_fd, n_name);
    create_new_user_by_nickname(user_db, n_name, new_sock_fd);
}