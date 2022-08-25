#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "file_io/file_io_copy.h"

// static const Command_T commandTable[]= {
//     {1, "hello"},
//     {2, "goodbye"}
// };

int main(int argc, char **argv){

    if(argc < 2){
        
        printf("Please check inputs\n");

        return 0;
    }

    int cmdId = atoi(argv[1]);
    printf("********************************\n");
    printf("cmdId: %d\n", cmdId);
    printf("********************************\n");

    switch(cmdId)
    {
        case 1:
            printf("Hello world!\n");
            break;
        case 2:
            printf("func_a: %d\n", func_a());
            break;
        default:
            printf("Can't find this command ID!\n");
            break;
    }

    return 0;
}