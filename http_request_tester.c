#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h> /* GNU libc manual 16.8, 16.9 - socket(), connect(), send(), receive()  */
#include <netdb.h> /* GNU libc manual 16.6.2.4 - struct hostent, gethostbyname */
#include <netinet/in.h> /* GNU libc manual 16.6.1 - struct sockaddr_in */
#include <arpa/inet.h> /* GNU libc manual 16.6.2.3 - inet_ntop */
#include <errno.h> /* error codes */
#include <unistd.h> /* read and write */

void print_help();

int main(int argc, char * argv[])
{
    /* arguemnt stuff */
    int c, option_index; 
    int is_get = 1;
    char * message = NULL;
    char * host_name = NULL;
    
    /* things we'll make later */
    struct hostent * server = NULL;
    struct sockaddr_in * server_addr = NULL;
    char ** str_iterator = NULL;
    char ip_address[INET_ADDRSTRLEN];
    char response[4096];
    int port_no = 80;
    int socket_no, status, total, sent, received, bytes; 
    
    while(1)
    {
        static struct option long_options[] = 
        {
            {"command", required_argument, 0, 'c'},
            {"message", required_argument, 0, 'm'},
            {"host_name", required_argument, 0, 'n'},
            {"port_no", required_argument, 0, 'p'}, 
            {"help", no_argument, 0, 'h'},
            {0,0,0,0}
        };
        c = getopt_long(argc, argv, "hc:m:n:p:", long_options, &option_index);
        
        if (c == -1)
            break;
        
        switch (c)
        {
            case 'c':
                if (strcmp(optarg, "GET") == 0)
                    is_get = 1;
                else
                    is_get = 0;
                break;
            case 'm':
                message = malloc(strlen(optarg));
                strcpy(message, optarg);
                break;
            case 'n':
                host_name = malloc(strlen(optarg));
                strcpy(host_name, optarg);
                break;
            case 'p':
                port_no = atoi(optarg);
                break;
            case 'h':
                print_help();
                return 0;
            case '?':
                /* getopt_long already printed an error message. */
                print_help();
                return 1;
            default:
                print_help();
                return 1;
        }
    }
    
    if(!message) //check for a null pointer message
    {
        printf("ERROR: Message is required\n");
        print_help();
        return 1;
    }
    
    /* find the server by host name */
    if(!host_name) //default host_name is "www.imsa.com"
    {
        host_name = malloc(strlen("www.imsa.com"));
        strcpy(host_name, "www.imsa.com");
    }
    server = gethostbyname(host_name);
    if(!server) 
    {
        switch(h_errno)
        {
            case HOST_NOT_FOUND:
                printf("ERROR HOST_NOT_FOUND: No such host is known.\n");
                return 1;
            case NO_DATA:
                printf("ERROR NO_DATA: The server recognized the request and the name, but no address is available. Another type of request to the name server for the domain might return an answer.\n");
                return 1;
            case NO_RECOVERY:
                printf("ERROR NO_RECOVERY: An unexpected server failure occurred which cannot be recovered.\n");
                return 1;
            case TRY_AGAIN:
                printf("ERROR TRY_AGAIN: A temporary and possibly transient error occurred, such as a failure of a server to respond.\n");
                return 1;
            default:
                printf("Unknown gethostbyname error\n");
                return 1;
        }   
        return 1;
    }
    else
    {
        printf("HOST FOUND by name \"%s\"\n", host_name);
        printf("Official Host Name: \"%s\"\n", server->h_name);
        if(server->h_aliases[0])
        {
            for(str_iterator = server->h_aliases; *str_iterator; str_iterator++)
            {
                printf("Host Alias: \"%s\"\n", *str_iterator);
            }
        }
        else
        {
            printf("no aliases listed\n");
        }
        if(server->h_addrtype == AF_INET)
        {
            printf("Address Type: IPv4\n");
        }
        else if(server->h_addrtype == AF_INET)
        {
            printf("Address Type: IPv6\n");
        }
        else
        {
            printf("Address type unknown (not IPv4 or IPv6... maybe you're looking up something weird)\n");
        }
        printf("Address length %d bytes\n", server->h_length);
        if(server->h_addr_list[0])
        {
            for(str_iterator = server->h_addr_list; *str_iterator; str_iterator++)
            {
                inet_ntop(server->h_addrtype, *str_iterator, ip_address, sizeof(ip_address));
                printf("IP Address: %s\n",ip_address);
            }
        }
        else
        {
            printf("no addresses listed\n");
        }
        printf("\n");
    }
    
    /* create the server address struct */
    server_addr = calloc(1, sizeof(struct sockaddr_in));
    server_addr->sin_family = server->h_addrtype;
    server_addr->sin_port = htons(port_no);
    memcpy(&(server_addr->sin_addr.s_addr),server->h_addr,server->h_length);
    
    /* now we're ready to connect the socket */
    printf("CONNECTING SOCKET TO HOST AT PORT %d...", port_no);
    socket_no = socket(PF_INET, SOCK_STREAM, 0); //TCP/IP connection
    if(socket_no == -1)
    {
        switch (errno)
        {
            case EPROTONOSUPPORT:
                printf("ERROR EPROTONOSUPPORT: The protocol or style is not supported by the namespace specified.\n");
                return 1;
            case EMFILE:
                printf("ERROR EMFILE: The process already has too many file descriptors open.\n");
                return 1;
            case ENFILE:
                printf("ERROR ENFILE: The system already has too many file descriptors open.\n");
                return 1;
            case EACCES:
                printf("ERROR EACCES: The process does not have the privilege to create a socket of the specified style or protocol.\n");
                return 1;
            case ENOBUFS:
                printf("ERROR ENOBUFS: The system ran out of internal buffer space.\n");
                return 1;
            default:
                printf("Unknown Socket Creation Error\n");
                return 1;
        }
    }
    
    /* connect the socket */
    if(connect(socket_no, (struct sockaddr *)server_addr, sizeof(server_addr)) == -1)
    {
        printf("socket connection error. See 16.9.1 Making a Connection for error descriptions\n.");
        return 1;
    }
    printf("Socket Connected Successfully\n");
    printf("\n");
    
    /* send the request */
    printf("SENDING THE MESSAGE\nMessage: %s", message);
    total = strlen(message);
    sent = 0;
    do {
        bytes = write(socket_no,message+sent,total-sent);
        if (bytes < 0)
        {
            printf("ERROR writing message to socket");
            return 1;
        }
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);
    printf("Message Sent\n");
    printf("\n");
    
    /* receive the response */
    printf("READING THE RESPONSE\n");
    memset(response,0,sizeof(response));
    total = sizeof(response)-1;
    received = 0;
    
    
    do {
        bytes = read(socket_no,response+received,total-received);
        if (bytes < 0)
        {
            printf("ERROR reading response from socket");
            return 1;
        }
        if (bytes == 0)
            break;
        received+=bytes;
    } while (bytes < total);
    
    if (received == total)
    {
        printf("The buffer ran out of space\n");
    }
    printf("Response:\n%s\n",response);
        
    
    /* free before we're done if it isn't null */
    if(message)
        free(message);
    
    if(host_name)
        free(host_name);
    
    if(server_addr)
        free(server_addr);
    
    status = shutdown(socket_no, 0);
    if(status == -1)
    {
        switch (errno)
        {
            case EBADF:
                printf("Socket Shutdown Error EBADF: socket is not a valid file descriptor.\n");
                return 1;
            case ENOTSOCK:
                printf("Socket Shutdown Error ENOTSOCK: socket is not a socket.\n");
                return 1;
            case ENOTCONN:
                printf("Socket Shutdown Error ENOTCONN: socket is not connected.\n");
                return 1;
            default:
                printf("unknown socket connection error\n.");
                return 1;
        }
    }
    
    return 0;
}

void print_help()
{
    printf("Usage: http_request_tester [options] -m <message>...\n");
    printf("Options:\n");
    printf("%15s ... %s\n","--help, -h","Display this prompt");
    printf("%15s ... %s\n","--command, -c","Specify type of command, e.g. POST or GET. GET is default");
    printf("%15s ... %s\n","--message, -m","Specify the message for the given command");
    printf("%15s ... %s\n","--host_name, -n","Format www.example.com, will lookup host address by name. www.imsa.com is default");
    printf("%15s ... %s\n","--port_no, -p","Port number entered as a number, e.g. \"-p 80\". Deault is 80 (HTTP)");
}