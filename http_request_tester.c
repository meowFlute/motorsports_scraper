#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h> /* GNU libc manual 16.6.2.4 - struct hostent, gethostbyname */
#include <netinet/in.h> /* GNU libc manual 16.6.1 - struct sockaddr_in */
#include <arpa/inet.h> /* GNU libc manual 16.6.2.3 - inet_ntop */

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
    
    while(1)
    {
        static struct option long_options[] = 
        {
            {"command", required_argument, 0, 'c'},
            {"message", required_argument, 0, 'm'},
            {"host_name", required_argument, 0, 'n'},
            {"help", no_argument, 0, 'h'},
            {0,0,0,0}
        };
        c = getopt_long(argc, argv, "hc:m:", long_options, &option_index);
        
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
                break;
            case NO_DATA:
                printf("ERROR NO_DATA: The server recognized the request and the name, but no address is available. Another type of request to the name server for the domain might return an answer.\n");
                break;
            case NO_RECOVERY:
                printf("ERROR NO_RECOVERY: An unexpected server failure occurred which cannot be recovered.\n");
                break;
            case TRY_AGAIN:
                printf("ERROR TRY_AGAIN: A temporary and possibly transient error occurred, such as a failure of a server to respond.\n");
                break;
            default:
                break;
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
    }
    
    /* create the server address struct */
    server_addr = calloc(1, sizeof(struct sockaddr_in));
    server_addr->sin_family = server->h_addrtype;
    server_addr->sin_port = htons(80);
    
    /* free before we're done if it isn't null */
    if(message)
        free(message);
    
    if(host_name)
        free(host_name);
    
    if(server_addr)
        free(server_addr);
    
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
}