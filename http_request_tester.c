#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

int main(int argc, char * argv[])
{
    int c, option_index; 
    int is_get = 1;
    char * message = NULL;
    
    while(1)
    {
        static struct option long_options[] = 
        {
            {"command", required_argument, 0, 'c'},
            {"message", required_argument, 0, 'm'},
            {0,0,0,0}
        };
        c = getopt_long(argc, argv, "c:m:", long_options, &option_index);
        
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
            case '?':
                /* getopt_long already printed an error message. */
                break;
            default:
                return 1;
        }
    }
    
    if(message)
        printf("is_get = %d\nmessage = %s\n", is_get, message);
    
    if(message)
        free(message);
    
    return 0;
}