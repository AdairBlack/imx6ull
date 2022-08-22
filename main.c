#include <cstdio>
#include <map>
#include "main.hpp"

using namespace std;

int main(int argc, char **argv){

    map<int, string> cmdsMap = {
        {(int)1, "cp file"},
        {(int)2, "mv file"}
    };

    if(argc < 2){
        printf("Please check inputs\n");
        map<int, string>::iterator cmdsMapIter = cmdsMap.begin();
        while(cmdsMapIter != cmdsMap.end()){
            printf("%d: %s\n", cmdsMapIter->first, cmdsMapIter->second.c_str());
        }
        return 0;
    }

    if(cmdsMap.find(atoi(argv[1])) == cmdsMap.end())
    {
        printf("Can't find the cmd!\n");
        return 0;
    }

    return 0;
}