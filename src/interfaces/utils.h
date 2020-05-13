
#ifndef CHIRC_UTILS_H
#define CHIRC_UTILS_H
#define MAX_NUM_OF_PARAMS_FOR_CMD 15


typedef struct Command{
    char cmd_string[100];
    char args[MAX_NUM_OF_PARAMS_FOR_CMD][100];
    int has_a_linked_command;
    struct Command *linked_command; // like NICK and USER are linked command because they are expected to be received together
    int cmd_filled_with_info;
} Command;

#endif //CHIRC_UTILS_H
