/*
* Copyright (c) 2012 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Qualcomm Technologies Proprietary and Confidential.
*/

#include "utils.h"
#include "debug.h"

#if !DEBUG_ON
#include "cutils/sockets.h"
#include "sys/socket.h"
extern "C"
{
#include "oncrpc.h"
#include "nv.h"
#include "nv_rpc.h"
}
#include "cutils/properties.h"
#endif

//MSM8X25_JB_2.0__SSRSKOLGM_2010D120927
string makeBuildVersion(const char *apVersion,const char* mpVersion)
{
    //MSM8X25_JB_2.0_H28094052_RUD
    const int ApSectionNum=5;
    const char ApDelim='_';
    //8X25-SSRSKOLGM-2010D120927
    const int MpSectionNum=3;
    const char MpDelim='-';

    string version;
    int begin=0,end=0,listIndex=0;
    // ap
    version=apVersion;
    /*    string apVersionString = apVersion;
    string apList[ApSectionNum];
    for(unsigned int i=0;i< strlen(apVersion);i++)
    {
        if(apVersion[i]==ApDelim || i== strlen(apVersion)-1)
        {
            end=i;
            if(i==0||begin!=0) begin+=1;
            if( i== strlen(apVersion)-1) end+=1;
            apList[listIndex++] = apVersionString.substr(begin,end-begin);
            begin=end;
        }
    }
    version=apList[0]+"_"+apList[1]+"_"+apList[2]+"_"+apList[4];*/
    // mp
    string mpVersionString=mpVersion;
    string mpList[MpSectionNum];
    listIndex=begin=end=0;    
    for(unsigned int i=0;i< strlen(mpVersion);i++)
    {
        if(mpVersion[i]==MpDelim || i== strlen(mpVersion)-1)
        {
            end=i;
            if(i==0||begin!=0) begin+=1;
            if( i== strlen(mpVersion)-1) end+=1;           
            mpList[listIndex++] = mpVersionString.substr(begin,end-begin);
            begin=end;
        }
    }
    version += "_"+ mpList[1]+"_"+mpList[2];
    return version;
}

#if !DEBUG_ON
static int setupNvRPC()
{
    logd("nvcb_app_init..");
    nvcb_app_init();

    oncrpc_init();
    oncrpc_task_start();
    return 0;
}
//uint8 sw_version_info[NV_MAX_SW_VERSION_INFO_SIZ];
static void readNv(nv_func_enum_type cmd, nv_items_enum_type item, nv_item_type *data_ptr)
{
    setupNvRPC();
    nv_cmd_remote(cmd, item, data_ptr);
}
#endif

void setBuildVersion()
{    
    char apVersion[50],mpVersion[50];
    memset(apVersion,0,50);
    memset(mpVersion,0,50);

    char AP_VERSION_PROPERTY[] = "ro.build.display.id";
    property_get(AP_VERSION_PROPERTY, apVersion, "ap");

    //this property is set after Android starts
    //so we must read from nv directly
    #if !DEBUG_ON
    nv_item_type data_ptr;
    readNv(NV_READ_F,NV_SW_VERSION_INFO_I, &data_ptr);
    memcpy(mpVersion,data_ptr.sw_version_info,NV_MAX_SW_VERSION_INFO_SIZ);
    char PropFile[]="/data/fota/ipth-muc.prop";
    char PropDir[]="/data/fota";
    #else
    char MP_VERSION_PROPERTY[] = "gsm.version.baseband";
    property_get(MP_VERSION_PROPERTY, mpVersion, "mp");
    char PropFile[]="G:\\data\\fota\\test.prop"; 
    char PropDir[]="G:\\data\\fota";
    #endif

    logd(apVersion);
    logd(mpVersion);

    string buildVersion=makeBuildVersion(apVersion,mpVersion);
    logd(buildVersion.c_str());

    // set ipth-muc.prop file
    mkDirs(PropDir);
    string content = "firmware.version="+buildVersion+"\n"
                        +"pkg.location=/cache/fota"+"\n"
                            +"max.pkg.size=41943040"+"\n";
    writeFile(PropFile,content.c_str());
    // set property
    property_set("persist.sys.mpversion",mpVersion);
}
