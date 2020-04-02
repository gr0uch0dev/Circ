/* A simple server in the internet domain using TCP
   The port number is passed as an argument */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;

    int n;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

/* The method w
 *
 *
 *
 */



    // Get the info using the C library

// getaddrinfo will get the result following the path of its last argument, in this case this is &res
// where &res is a pointer to a pointer to a struct addrinfo
// this is to say that accordingly to the other arguments the procedure will know where to look for


// writing a socket is like giving the information of the building we live in

// binding the socket is providing also the info of our house door

// the kernel works in this case as a postman that arrives straight in front of the door




    // TODO Version 2.0 create the server using getaddrinfo to get the information
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
    if ( (status =getaddrinfo(NULL,argv[1], &hints, &res)) == -1){
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

    while (1){
        int new_sock_fd;
        new_sock_fd = accept(socket_fd, (struct sockaddr *) &client_sock_addr, (socklen_t *) &client_len);

        // TODO change the write and read in send and recv
        bzero(buffer,256);
        n = recv(new_sock_fd, buffer, 256, 0);
        //n = read(new_sock_fd,buffer,255);
        if (n < 0) error("ERROR reading from socket");

        printf("Here is the message: %s\n",buffer);
        n = send(new_sock_fd, buffer, 256, 0);
        //n = write(new_sock_fd,"I got your message",18);
        if (n < 0) error("ERROR writing to socket");
        close(new_sock_fd);

    }


    // host = NULL because we want to use our localhost
    // and as char *service we pass the port given as argument by the user
    return 0;

}



// The version commented is the old way of approaching this problem

// we use getaddress to translate given information in socket addresses
//    // This is the old way to create and bind the socket. The new way (following) uses getaddrinfo
//    // socket asks for a protocol family (e.g. PF_INET) although this is equal to AF_INET (interchangeable)
//    sockfd = socket(PF_INET, SOCK_STREAM, 0);
//    if (sockfd < 0)
//        error("ERROR opening socket");
//    bzero((char *) &serv_addr, sizeof(serv_addr));
//    portno = atoi(argv[1]);
//    serv_addr.sin_family = AF_INET;
//    serv_addr.sin_addr.s_addr = INADDR_ANY;
//    serv_addr.sin_port = htons(portno);

// ################################
// ################################
// ################################
// An other way is to use the information got from getaddrinfo()
//    int s;
//    struct addrinfo hints, *res; // the addrinfo is a struct part of a linked list with all the relevant information
//                                // that we need
//
//    char *host = "www.example.com";
//    int status;
//    if ((status = getaddrinfo(host, "http", &hints, &res)) != 0){
//        fprintf(stderr, "Error in getaddrinfo: %s", gai_strerror(status));
//        return 1;
//    }





//    if (bind(sockfd, (struct sockaddr *) &serv_addr,
//             sizeof(serv_addr)) < 0)
//        error("ERROR on binding");
//
//    listen(sockfd,5);
//    clilen = sizeof(cli_addr);

//    while (1) {
//        newsockfd = accept(sockfd,
//                           (struct sockaddr *) &cli_addr, &clilen);
//        if (newsockfd < 0)
//            error("ERROR on accept");
//
//        bzero(buffer,256);
//        n = read(newsockfd,buffer,255);
//        if (n < 0) error("ERROR reading from socket");
//
//        //printf("Here is the message: %s\n",buffer);
//
//        n = write(newsockfd,"I got your message",18);
//        if (n < 0) error("ERROR writing to socket");
//        close(newsockfd);
//    }
