
#ifndef CHIRC_UTILS_H
#define CHIRC_UTILS_H
#define MAX_NUM_OF_PARAMS_FOR_CMD 15
void update_words_received(int n, const char *buffer, char words_receieved[100][100], int *count_words);

struct InfoCommands{
    int nick_cmd_sent;
    int user_cmd_sent;
};


typedef struct Command{
    char cmd_string[100];
    char args[MAX_NUM_OF_PARAMS_FOR_CMD][100];
    int has_a_linked_command;
    struct Command *linked_command; // like NICK and USER are linked command because they are expected to be received together
    int cmd_filled_with_info;
} Command;

#endif //CHIRC_UTILS_H
