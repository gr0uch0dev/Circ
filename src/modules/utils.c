//
// Created by groucho on 05/04/20.
//

#include <../src/interfaces/utils.h>

void update_words_received(int n, const char *buffer, char words_receieved[100][100], int *count_words){
    int counter = n, i = 0, j=0;

    while(counter > i) {
        if (buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != ' '){
            words_receieved[*count_words][j] = buffer[i];
            j++;
            if (buffer[i+1] == ' ' || buffer[i+1] == '\n' || buffer[i+1] == '\r'){
                words_receieved[*count_words][j] = '\0';
                (*count_words)++;
                j = 0;
            }
        }
        i++;
    }
}