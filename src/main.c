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



void error(char *msg) {
    perror(msg);
    exit(1);
}

// to export these
void parse_msg_for_cmd_and_args(const char *input_baffer, char cmd_and_args[100][100]);
void create_new_user_with_the_data_got(int new_sock_fd, User *user_db, char *n_name, char *u_name);
void process_the_command(int socket, User *user_db, Command);
Command build_the_command(char cmd_and_args[100][100]);
int are_linked_commands(Command first_command, Command second_command);



void send_message_to_client(char *buffer, int socket_fd){
    int n;
    n = send(socket_fd, buffer, 256, 0);
    chilog(INFO,"Sent to socket: %s\n",buffer);
    if (n < 0) error("ERROR writing to socket");
}

void send_greetings(int socket_fd, User input_user){
    char buffer[256];

    bzero(buffer, 256);
    build_numeric_reply("WELCOME", buffer, input_user);
    send_message_to_client(buffer, socket_fd);

    bzero(buffer, 256);
    build_numeric_reply("YOUR_HOST", buffer, input_user);
    send_message_to_client(buffer, socket_fd);

    bzero(buffer, 256);
    build_numeric_reply("CREATED", buffer, input_user);
    send_message_to_client(buffer, socket_fd);

    bzero(buffer, 256);
    build_numeric_reply("MY_INFO", buffer, input_user);
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

        // TODO understand
        char *buffer_with_cmd_and_args = (char *) malloc(512);
//        char buffer_with_cmd_and_args[512];
        int num_chars_got = 0;
        Command received_cmd = {};
        Command previous_cmd = {};
        bzero(&previous_cmd, sizeof(previous_cmd));
        int command_can_be_processed = 0;
        //char *buffer = (char *) malloc(256);
        // TODO: understand why if we use malloc we have problem with chilog

        while(1){
            // msg delimeted by CRLF
            bzero(buffer,256);
            n = recv(new_sock_fd, buffer, 255, 0);
            if (num_msg == 1){
                int x = 0; // debug
            }
            chilog(INFO, "Got message #%d of length %d. Read: %s",++num_msg, n, buffer);

            //if (n < 0) error("Error in reading from socket");
            if (n == 0) break;
            if (n == -1){
                perror("Error reading from socket");
            }

            // the logic is: get all the words up to \r\n. As soon as CRLF is found execute the command!

            // command1: [par1, par2] 3 levels of depth
            // command2: [par1, par2]
            int i = 0;
            while((current_read_char = buffer[i++]) != 0){ // until we have sth to read in the buffer
                if (last_read_char == '\r' && current_read_char == '\n'){
                    // got end of the message
                    // all the words up to now are parts of a COMMAND + [Args]
                    bzero(cmd_and_args_array, sizeof(cmd_and_args_array));
                    // buffer_with_cmd_and_args -> holds all the chars up to CRLF
                    // we get all the words in cmd_and_args_array
                    parse_msg_for_cmd_and_args(buffer_with_cmd_and_args, cmd_and_args_array);
                    bzero(buffer_with_cmd_and_args, 512);

                    bzero(&received_cmd, sizeof(received_cmd));
                    received_cmd = build_the_command(cmd_and_args_array);

                    // check if the command can be processed
                    if (!received_cmd.has_a_linked_command){
                        command_can_be_processed = 1;
                    } else{
                        command_can_be_processed = 0;
                        if (previous_cmd.cmd_filled_with_info && are_linked_commands(received_cmd, previous_cmd)){ // if it is linked to a command that already has info
                            // link to previous
                            received_cmd.linked_command = &previous_cmd;
                            command_can_be_processed = 1;
                        }
                    }

                    // act if the command can be processed
                    if (command_can_be_processed){
                        process_the_command(new_sock_fd, p_user_head, received_cmd);
                        //clean_the_command_buffer;
                        bzero(&previous_cmd, sizeof(previous_cmd));

                    } else{
                        previous_cmd = received_cmd;
                    }
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
    // chilog(INFO,"Handle this problem. We just got CRLF") -> results in error
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

//void process_the_command(int socket, User *user_db, char cmd_and_args[100][100]){
//    // consider the various possible commands
//    if (strncmp(cmd_and_args[0], "NICK", 5) == 0){ // this works only if NICK is received earlier (TODO fix this with struct)
//        if (strncmp(cmd_and_args[2], "USER", 4) != 0){
//            perror("USER is expected after NICK command.... (TODO adjustments)");
//        }
//        User a_new_user = create_new_user(socket, user_db, cmd_and_args[1], cmd_and_args[3]);
//        send_greetings(socket, a_new_user);
//
//    } else{
//        chilog(INFO, "command yet to be implemented");
//    }
//
//}

void process_the_command(int socket, User *user_db, Command cmd_info){
    // consider the various possible commands
    if (cmd_info.has_a_linked_command == 1){
        if (!cmd_info.linked_command) perror("An expected linked command was not provided!");
    }
    char nick_name[100], user_name[100];
    bzero(nick_name, 100);
    bzero(user_name, 100);

    if (strncmp(cmd_info.cmd_string, "NICK",4) == 0){
        strncpy(nick_name, cmd_info.args[0], 100);
        strncpy(user_name, cmd_info.linked_command->args[0], 100);
    }
    if (strncmp(cmd_info.cmd_string, "USER",4) == 0){
        strncpy(user_name, cmd_info.args[0], 100);
        strncpy(nick_name, cmd_info.linked_command->args[0], 100);
    }

    User a_new_user = create_new_user(socket, user_db, nick_name, user_name);
    send_greetings(socket, a_new_user);
}


Command build_the_command(char cmd_and_args[MAX_NUM_OF_PARAMS_FOR_CMD+1][100]){
    Command cmd_info = {};
    bzero(&cmd_info, sizeof(cmd_info));
    // storing the command got
    strncpy(cmd_info.cmd_string, cmd_and_args[0], 100);
    if (strncmp(cmd_info.cmd_string, "NICK", 4) == 0) cmd_info.has_a_linked_command = 1;
    if (strncmp(cmd_info.cmd_string, "USER", 4) == 0) cmd_info.has_a_linked_command = 1;

    // storing the arguments provided
    int i = 0;
    char empty_array[100];
    bzero(empty_array, 100);
    while (strncmp(cmd_and_args[i + 1], empty_array, 100) != 0){
        // argument to save
        strncpy(cmd_info.args[i], cmd_and_args[i+1], 100);
        i++;
    }
    cmd_info.linked_command = NULL;
    cmd_info.cmd_filled_with_info = 1;
    //cmd_info.args = cmd_and_args // check but this should make the array points to the next element
    return cmd_info;
}


int are_linked_commands(Command first_command, Command second_command){
    if (strncmp(first_command.cmd_string, "NICK", 4) == 0){
        return (strncmp(second_command.cmd_string, "USER", 4) == 0);
    }
    if (strncmp(first_command.cmd_string, "USER", 4) == 0){
        return (strncmp(second_command.cmd_string, "NICK", 4) == 0);
    }
    return 0;
}