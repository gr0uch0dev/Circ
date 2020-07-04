//
// Created by groucho on 05/04/20.
//

#include <../src/interfaces/utils.h>
#include <string.h>
#include <stdlib.h>
#include <log.h>
// TODO better to use a data driven methodology like
// {
//   "WELCOME": welcome_procedure
//   "YOUR_HOST": welcome_your_host
// }


void build_rpl_welcome(char *output_buffer, User receiver_user){
    sprintf(output_buffer, ":circ.groucho.com %s %s :Welcome to the Internet Relay Network %s!%s@user.example.com\r\n", // do not leave empty space before CRLF
            RPL_WELCOME, receiver_user.nick_name, receiver_user.nick_name, receiver_user.user_name);
}

void build_rpl_your_host(char *output_buffer, User receiver_user){
    sprintf(output_buffer, ":circ.groucho.com %s %s :Your host is <servername>, running version <ver>\r\n",
            RPL_YOURHOST, receiver_user.nick_name);
}

void build_rpl_created(char *output_buffer, User receiver_user){
    sprintf(output_buffer, ":circ.groucho.com %s %s :This server was created <date>\r\n",
            RPL_CREATED, receiver_user.nick_name);
}

void build_rpl_my_info(char *output_buffer, User receiver_user){
    sprintf(output_buffer, ":circ.groucho.com %s %s :<servername> <version> ao mtov\r\n",
            RPL_MYINFO, receiver_user.nick_name);
}

void build_numeric_reply(char *mode, char *output_buffer, User receiver_user){
    // store in output_buffer the msg buffer to be sent to the connected user
    int len_mode_str = strnlen(mode,50);
    if ((strncmp(mode, "WELCOME", len_mode_str) == 0)) build_rpl_welcome(output_buffer, receiver_user);
    if ((strncmp(mode, "YOUR_HOST", len_mode_str) == 0)) build_rpl_your_host(output_buffer, receiver_user);
    if ((strncmp(mode, "CREATED", len_mode_str) == 0)) build_rpl_created(output_buffer, receiver_user);
    if ((strncmp(mode, "MY_INFO", len_mode_str) == 0)) build_rpl_my_info(output_buffer, receiver_user);
    if (!output_buffer){
        chilog(INFO, "ERROR TO MANAGE");
        exit(-1);
    }

}

