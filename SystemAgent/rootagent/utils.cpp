/*
 * Copyright (c) 2012, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 * Developed by QRD Engineering team.
 */
#include "utils.h"
void logd(int d)
{
    printf("%d\n",d);
}
void logd(char* s)
{
    printf("%s\n",s);
}
void logd(char c)
{
    printf("%c\n",c);
}
void logd(string s)
{
   printf("%s\n",s.c_str());
}

#if !DEBUG_ON
// can make parent directories 
int mkDirs(const char *path)
{
    int len = strlen(path);
    char* dir = new char[len+1];
    strcpy(dir, path);
    int i;
    for(i=0; i<len; i++ )
    {
        if(i>0 && dir[i]=='/')
        {
            dir[i] = '\0';
            if(access(dir,F_OK))
            {
                if(mkdir(dir, 0777))
                {
                    delete[] dir;
                    return -1;
                }
            }
            dir[i]='/';
        }
    }
    if(access(dir, F_OK))
    {
        if(mkdir(dir, 0777))
        {
            delete[] dir;
            return -1;
        }
    }
    delete[] dir;
    return 0;
    //string cmd = "mkdir -p " + string(path);
    //return system(cmd.c_str());
}
#else
int mkDirs(const char *path)
{
    string cmd = "mkdir " + string(path);
    return system(cmd.c_str());
}
#endif

void writeFile(const char* path, const char* content )
{
    #if !DEBUG_ON
    umask(0000);
    #endif
    FILE* fp = fopen(path,"w");    
    if(fp==NULL)
    {        
        logd("open "+std::string(path)+" failed");
        perror(path);
        return;
    }
    fwrite(content,sizeof(char),strlen(content),fp);
    fclose(fp);
}

int runCommand(const char* cmd)
{
    int ret = system(cmd);
    return ret;
}
