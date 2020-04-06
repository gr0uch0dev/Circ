//
// Created by groucho on 05/04/20.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <interfaces/user.h>

#include <log.h>

//User *current_user = NULL;

void add_next_in_linked_struct(User *p_user, User new_user){
    if(!(p_user -> next)) {
        p_user -> next = &new_user;
    } else{
        add_next_in_linked_struct(p_user -> next, new_user);
    }

}

void create_new_user_by_nickname(User * p_user_head, char *nick_name, int socket_fd) {
    // represents the users with a linked data structure


    User *p_current_user = p_user_head;

    if (!(p_current_user->nick_name)){
        // first user needs to be created
        p_user_head -> nick_name = nick_name;
        p_user_head -> socket_fd = socket_fd;
        p_user_head -> next = NULL;
    } else{
        while(p_current_user -> next){
            p_current_user = p_current_user -> next;
        }
        p_current_user -> next = (User *) malloc(sizeof(User));
        bzero(p_current_user -> next, sizeof(User));

        (p_current_user -> next) -> nick_name = nick_name;
        (p_current_user -> next) -> socket_fd = socket_fd;
        (p_current_user -> next) -> next = NULL;
    }


}
//



void print_all_nicknames(User * p_user_head){
    User *p_current_user = p_user_head;

    if (!p_current_user-> nick_name){
        chilog(INFO,"No User to print");
    } else{
        while(p_current_user -> next){
            chilog(INFO, "User with nickname: %s\n", p_current_user -> nick_name);
            p_current_user = p_current_user -> next;
        }
    }

}