//
// Created by groucho on 05/04/20.
//

#ifndef CHIRC_USER_H
#define CHIRC_USER_H

typedef struct User{
    int socket_fd;
    char *nick_name;
    char *email;
    struct User *next;

}User;


void create_new_user_by_nickname(User * head, char *nick_name, int socket_fd);
void print_all_nicknames(User* p_head);
User *remove_user_by_nickname(User* p_user_head, char *n_name_to_remove);

#endif //CHIRC_USER_H
