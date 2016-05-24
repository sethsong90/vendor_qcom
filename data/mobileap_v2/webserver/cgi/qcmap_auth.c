/******************************************************************************

                           QCMAP_AUTH.C

******************************************************************************/

/******************************************************************************

  @file    qcmap_auth.c
  @brief   Mobile AP Web Authentication Module Implementation

  DESCRIPTION
  Mobile AP Web Authentication Module Implementation

  ----------------------------------------------------------------------------
  Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ----------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

when       who        what, where, why
--------   ---        --------------------------------------------------------
04/04/12   vb         Initial Autorization bringup.
08/30/12   at         Added Timeout update and unecode of password.
******************************************************************************/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

//File used to store user credentials
#define PASSWORD_FILE "/etc/lighttpd.user"
#define SESSION_FILE "/etc/qcmap_session"
//Default and only user name for accessing web server
#define DEFAULT_USER_NAME "admin"
//Returns Minimum value of both
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MAX_HTTP_BUFFER_SIZE 100       //Max Buffer size for HTTP Buffer
#define MAX_ELEMENT_LENGTH 45          //Max Size of any element value
#define MAX_ELEMENT_COUNT 4            //Max Elements can be processed
#define IPV4_STRING_LEN 16             //Max string length of IPV4 address
#define PASSWORD_LENGTH 16             //Max password length + 1
#define PASSWORD_POSITION 6
#define CHANGE_PWD_SUCCESS "Content-Type: text/html\nContent-Length: \
                            24\n\n{\"result\":\"0\", \"ip\":\"0\"}"
#define FIRST_LOGIN_SUCCESS "Content-Type: text/html\nContent-Length: \
                            24\n\n{\"result\":\"2\", \"ip\":\"0\"}"
#define LOGIN_FAIL "Content-Type: text/html\nContent-Length: \
                            24\n\n{\"result\":\"3\", \"ip\":\"0\"}"
#define DEFAULT_LOGIN "Content-Type: text/html\nContent-Length: \
                            24\n\n{\"result\":\"4\", \"ip\":\"0\"}"
#define FILE_READ_ERR "Content-Type: text/html\nContent-Length: \
                       15\n\nFile Read Error"
#define FILE_CRED_ERR "Content-Type: text/html\nContent-Length: \
                       19\n\nCredentials Missing"
#define FILE_WRITE_ERR "Content-Type: text/html\nContent-Length: \
                       16\n\nFile Write Error"
#define FILE_OPEN_ERR "Content-Type: text/html\nContent-Length: \
                       15\n\nFile Open Error"
#define SYSTEM_ERR "Content-Type: text/html\nContent-Length: \
                       12\n\nSystem Error"
#define GOT_NOTHING "Content-Type: text/html\nContent-Length: 7\n\n\
                     Nothing"
#define UNENCODE_ERR "Content-Type: text/html\nContent-Length: \
                       14\n\nunencode Error"
#define HTML_HEADER "Content-Type: text/html\nContent-Length: "
#define LOG_BUFF_SIZE 28 //Size of log structure
#define SUCCESS 0
#define FAIL    -1

typedef struct
{
  //To store last login time
  struct timeval log_time;
  //To store last login login IP
  char ip[15];
  //To specify session timeout
  unsigned int timeout;
}loginfo;
void http_parser(char *http_cgi_buff,
                 char http_element_name[][MAX_ELEMENT_LENGTH],
                 char http_element_val[][MAX_ELEMENT_LENGTH],
                 int elements);

int main(void)
{
  //To parse incoming HTTP Traffic Length.
  char *http_inc_traffic_len = NULL;
  int DEFAULT_TIMEOUT  = 300;
  char DEFAULT_IP[IPV4_STRING_LEN] = {0};
  strlcpy(DEFAULT_IP, "0.0.0.0",sizeof("0.0.0.0"));
  //To store Min buffer length required to read HTTP, afterwards this
  // variable can be used as temp buffer.
  int html_cgi_buff_len = 0;
  //To Generate HTTP Request.
  char cgi_html_buff[MAX_HTTP_BUFFER_SIZE]={0};
  //To store Min buffer length required to read HTTP, afterwards this
  //variable can be used as temp. buffer.
  int cgi_html_buff_len = 0;
  char html_cgi_buff[MAX_HTTP_BUFFER_SIZE]={0}; //To store HTTP Request.
  FILE *fp;  //File pointer to Password File.
  // To store individual Element Names.
  char http_element_name[MAX_ELEMENT_COUNT][MAX_ELEMENT_LENGTH] = {0};
  // To Store individual Element values.
  char http_element_val[MAX_ELEMENT_COUNT][MAX_ELEMENT_LENGTH] = {0};
  //To store credentials read from user credentials file.
  char pwd[PASSWORD_LENGTH] = {0};
  //Time stamp of the system
  struct timeval sys_time;
  //To store log message read from file
  loginfo *linfo;
  //To store log message written to file
  loginfo sinfo;
  //To get client IP address
  char remote_ip[IPV4_STRING_LEN] = {0};
  //To store return values from system calls for comparition
  int res = 0;
  char *remote_addr = NULL;

  http_inc_traffic_len = getenv("CONTENT_LENGTH");

  //Get clients IP address
  if (http_inc_traffic_len != NULL)
  {
    //Initialize memories to hold data.
    remote_addr = getenv("REMOTE_ADDR");
    if(remote_addr !=NULL)
    {
      strncat(remote_ip, remote_addr, IPV4_STRING_LEN - 1);
      memset(html_cgi_buff, 0, MAX_HTTP_BUFFER_SIZE);
      memset(cgi_html_buff, 0, MAX_HTTP_BUFFER_SIZE);
      memset(&sinfo, 0, LOG_BUFF_SIZE);
      html_cgi_buff_len = atoi(http_inc_traffic_len);
      if (html_cgi_buff_len > 0)
      {
        // Avoid buffer overflow
        html_cgi_buff_len = MIN(html_cgi_buff_len,
                                sizeof(html_cgi_buff)-1 );
        //Read incoming stream data into html_cgi_buff
        fread(html_cgi_buff, 1, html_cgi_buff_len, stdin );
      }
      else
      {
        return FAIL;
      }
      //Send fetched data to parse.
      http_parser(html_cgi_buff, http_element_name,
                  http_element_val, MAX_ELEMENT_COUNT);
      //If request for login
      if (!strncmp("login",http_element_val[0],MAX(strlen("login"),strlen(http_element_val[0]))))
      {
        //Open file for reading previous request timestamp
        fp = fopen(SESSION_FILE, "r");
        if (fp == NULL)
          fp = fopen(SESSION_FILE, "w+");
        memset(html_cgi_buff, 0, MAX_HTTP_BUFFER_SIZE);
        if (fp != NULL)
        {
          //Read previous timestamp
          res = fread(html_cgi_buff,1,LOG_BUFF_SIZE,fp);
          //Close file after reading for reopening it in write mode
          fclose(fp);
          if (res != LOG_BUFF_SIZE && res != 0)
          {
            printf("%s",FILE_READ_ERR);
            return FAIL;
          }
          linfo = (loginfo *)html_cgi_buff;
          if (strlen(html_cgi_buff) > 0 && linfo->timeout > 0)
          {
            // check for session ip to be default ip re-login page
            if ((!strncmp(linfo->ip,DEFAULT_IP,strlen(linfo->ip))))
            {
              printf ("%s",DEFAULT_LOGIN);
              return FAIL;
            }
            //Get timestamp from system
            gettimeofday(&sys_time, 0);
            //Check the timestamp if it is in the time out window
            //Previous timestamp - Present timestamp should be greater than timeout time.
            if ((sys_time.tv_sec - linfo->log_time.tv_sec) > linfo->timeout)
            {
              fp = fopen(SESSION_FILE, "w+");
              if (fp != NULL)
              {
                //Write values in appropriate structure members
                //and write the struct data in to file
                strncat(sinfo.ip, remote_ip,strlen(remote_ip));
                sinfo.log_time.tv_sec = sys_time.tv_sec;
                sinfo.log_time.tv_usec = sys_time.tv_usec;
                sinfo.timeout = linfo->timeout;
                res = fwrite((char *)&sinfo,1,LOG_BUFF_SIZE,fp);
                fclose(fp);
                if (res == LOG_BUFF_SIZE)
                  printf("%s%d\n\n{\"result\":\"0\",\"ip\":\"0\",\"timeout\":\"%d\"}",HTML_HEADER,(int)(36+sizeof(sinfo.timeout)),sinfo.timeout);
                else
                {
                  printf("%s",FILE_WRITE_ERR);
                }
              }
              else
              {
                printf("%s",FILE_OPEN_ERR);
              }
            }
            //Previous timestamp - Present timestamp is not greater than timeout time.
            else
            {
              //Check if the IP of previous timestamp is same as present request IP to allow access
              // Also in case of logout it configured to DEFAULT, so allow
              if ((!strncmp(linfo->ip,remote_ip,strlen(linfo->ip))) ||
                  (!strncmp(linfo->ip,DEFAULT_IP,strlen(linfo->ip))))
              {
                fp = fopen(SESSION_FILE, "w+");
                if (fp != NULL)
                {
                  //If previous Timestamp IP is same as present request IP update timestamp info.
                  strncat(sinfo.ip, remote_ip,strlen(remote_ip));
                  sinfo.log_time.tv_sec = sys_time.tv_sec;
                  sinfo.log_time.tv_usec = sys_time.tv_usec;
                  sinfo.timeout = linfo->timeout;
                  res = fwrite((char *)&sinfo,1,LOG_BUFF_SIZE,fp);
                  fclose(fp);
                  if (res == LOG_BUFF_SIZE)
                    printf("%s%d\n\n{\"result\":\"0\", \"ip\":\"%s\",\"timeout\":\"%d\"}",HTML_HEADER,(int)(36+strlen(linfo->ip)+sizeof(linfo->timeout)),linfo->ip,linfo->timeout);
                  else
                  {
                    printf("%s",FILE_WRITE_ERR);
                  }
                }
                else
                {
                  printf("%s",FILE_OPEN_ERR);
                }
              }
              else
              {
                printf("%s%d\n\n{\"result\":\"1\", \"ip\":\"%s\"}",HTML_HEADER,(int)(23+strlen(linfo->ip)),linfo->ip);
              }
            }
          }
          //If it is very first login(like brand new modem)
          else
          {
            // Return to  HTML page to redirect user for change password!
            printf("%s",FIRST_LOGIN_SUCCESS);
          }
        }
        else
        {
          printf("%s",FILE_CRED_ERR);
        }
      }
      //If request is to update password
      else if (!strncmp("update",http_element_val[0],MAX(strlen("update"),strlen(http_element_val[0]))))
      {
        //Open file containing password to read.
        fp = fopen(PASSWORD_FILE, "r");
        //If password file opened successfully
        if (fp != NULL)
        {
          memset(pwd,0,PASSWORD_LENGTH);
          //Read contents of the password file
          fseek(fp,PASSWORD_POSITION,SEEK_SET);
          res = fread(pwd,1,(PASSWORD_LENGTH - 1),fp);
          if (res > 0)
            pwd[res] = 0;
          fclose(fp);
          //Compare if given old password is valid after unencode
          if(unencode_string( http_element_val[1],strlen(http_element_val[1]))
             == FAIL)
          {
            printf("%s",UNENCODE_ERR);
          }
          else
          {
            if (strlen(pwd) > 0 && !strncmp(pwd,http_element_val[1],MAX(strlen(pwd),strlen(http_element_val[1]))))
            {
              fp = fopen(PASSWORD_FILE, "w+");
              if (fp != NULL)
              {
                //If given old password is valid update it with new password.
                // Before update unencode password
                if(unencode_string( http_element_val[2],strlen(http_element_val[2]))
                   == FAIL)
                {
                  printf("%s",UNENCODE_ERR);
                }
                else
                {

                  fprintf(fp, "%s:%s",DEFAULT_USER_NAME,http_element_val[2]);
                  fclose(fp);
                  // Also update timeout as entered by the user
                  fp = fopen(SESSION_FILE, "w+");
                  if (fp != NULL)
                  {
                    //Read system time just to fill in structure
                    if (gettimeofday(&sys_time, 0) == 0)
                    {
                      strncat(sinfo.ip, remote_ip,strlen(remote_ip));
                      sinfo.log_time.tv_sec = sys_time.tv_sec;
                      sinfo.log_time.tv_usec = sys_time.tv_usec;
                      sinfo.timeout = atoi(http_element_val[3]);
                      //update timeout value in structure inturn updating it in file
                      res = fwrite((char *)&sinfo,1,LOG_BUFF_SIZE,fp);
                      fclose(fp);
                      if(res == LOG_BUFF_SIZE)
                      {
                        //Send Success HTTP responce
                        printf("%s",CHANGE_PWD_SUCCESS);
                      }
                      else
                      {
                        //Send failure HTTP responce
                        printf("%s",FILE_OPEN_ERR);
                      }
                    }
                    else
                    {
                      fclose(fp);
                      printf("%s",SYSTEM_ERR);
                    }
                  }
                  else
                  {
                   //Send failure HTTP responce
                   printf("%s",FILE_OPEN_ERR);
                  }

                }
              }
              else
              {
                printf("%s",FILE_OPEN_ERR);
              }
            }
            else
            {
              //Close File and send failure HTTP responce
              fclose(fp);
              printf("%s",FILE_READ_ERR);
            }
          }
        }
        else
        {
          //Send failure HTTP responce
          printf("%s",FILE_OPEN_ERR);
        }
      }
      //If the request is for update time out only
      else if (!strncmp("timeupdate",http_element_val[0],MAX(strlen("timeupdate"),strlen(http_element_val[0]))))
      {
        //update timeout as entered by the user
        fp = fopen(SESSION_FILE, "w+");
        if (fp != NULL)
        {
          //Read system time just to fill in structure
          if (gettimeofday(&sys_time, 0) == 0)
          {
            strncat(sinfo.ip, remote_ip,strlen(remote_ip));
            sinfo.log_time.tv_sec = sys_time.tv_sec;
            sinfo.log_time.tv_usec = sys_time.tv_usec;
            sinfo.timeout = atoi(http_element_val[1]);
            //update timeout value in structure inturn updating it in file
            res = fwrite((char *)&sinfo,1,LOG_BUFF_SIZE,fp);
            fclose(fp);
            if(res == LOG_BUFF_SIZE)
            {
              //Send Success HTTP responce
              printf("%s",CHANGE_PWD_SUCCESS);
            }
            else
            {
              //Send failure HTTP responce
              printf("%s",FILE_OPEN_ERR);
            }
          }
          else
          {
            fclose(fp);
            printf("%s",SYSTEM_ERR);
          }
        }
        else
        {
          //Send failure HTTP responce
          printf("%s",FILE_OPEN_ERR);
        }
      }
      //If the request is for logout or close session forcefully
      else if (!strncmp("close",http_element_val[0],MAX(strlen("close"),strlen(http_element_val[0]))))
      {
        //Get timestamp from system
        gettimeofday(&sys_time, 0);
        fp = fopen(SESSION_FILE, "w+");
        if (fp != NULL)
        {
          // configure default ip in session file
          strncat(sinfo.ip, DEFAULT_IP,IPV4_STRING_LEN - 1);
          sinfo.log_time.tv_sec = sys_time.tv_sec;;
          sinfo.log_time.tv_usec = sys_time.tv_usec;
          sinfo.timeout = atoi(http_element_val[1]);
          fwrite((char *)&sinfo,1,LOG_BUFF_SIZE,fp);
          fclose(fp);
          printf("%s",CHANGE_PWD_SUCCESS);
        }
        else
        {
          //Send failure HTTP responce
          printf("%s",FILE_OPEN_ERR);
        }
      }
      //If request is to Re-login from login page
      else if (!strncmp("session",http_element_val[0],MAX(strlen("session"),strlen(http_element_val[0]))))
      {
        //Open file containing password to read.
        fp = fopen(PASSWORD_FILE, "r");
        //If password file opened successfully
        if (fp != NULL)
        {
          memset(pwd,0,PASSWORD_LENGTH);
          //Read contents of the password file
          fseek(fp,PASSWORD_POSITION,SEEK_SET);
          res = fread(pwd,1,(PASSWORD_LENGTH - 1),fp);
          if (res > 0)
            pwd[res] = 0;
          fclose(fp);
          //Compare if given password is valid
          // Before comparison unencode password
          if(unencode_string( http_element_val[1],strlen(http_element_val[1]))
             == FAIL)
          {
            printf("%s",UNENCODE_ERR);
          }
          else
          {
            if (strlen(pwd) > 0 && !strncmp(pwd,http_element_val[1],MAX(strlen(pwd),strlen(http_element_val[1]))))
            {
              //Open file for reading previous request timestamp
              fp = fopen(SESSION_FILE, "r");
              if (fp == NULL)
                fp = fopen(SESSION_FILE, "w+");
              memset(html_cgi_buff, 0, MAX_HTTP_BUFFER_SIZE);
              if (fp != NULL)
              {
                //Read previous timestamp
                res = fread(html_cgi_buff,1,LOG_BUFF_SIZE,fp);
                //Close file after reading for reopening it in write mode
                fclose(fp);
                if (res != LOG_BUFF_SIZE && res != 0)
                {
                  printf("%s",FILE_READ_ERR);
                  return FAIL;
                }
                linfo = (loginfo *)html_cgi_buff;
                // update if no other ip is using webserver
                if ((!strncmp(linfo->ip,remote_ip,strlen(linfo->ip))) ||
                   (!strncmp(linfo->ip,DEFAULT_IP,strlen(linfo->ip))))
                {
                  // Also update timeout to default
                  fp = fopen(SESSION_FILE, "w+");
                  if (fp != NULL)
                  {
                    //Read system time just to fill in structure
                    if (gettimeofday(&sys_time, 0) == 0)
                    {
                      strncat(sinfo.ip, remote_ip,strlen(remote_ip));
                      sinfo.log_time.tv_sec = sys_time.tv_sec;
                      sinfo.log_time.tv_usec = sys_time.tv_usec;
                      sinfo.timeout = linfo->timeout;
                      //update timeout value in structure inturn updating it in file
                      res = fwrite((char *)&sinfo,1,LOG_BUFF_SIZE,fp);
                      fclose(fp);
                      if(res == LOG_BUFF_SIZE)
                      {
                        //Send Success HTTP responce
                        printf("%s",CHANGE_PWD_SUCCESS);
                      }
                      else
                      {
                        //Send failure HTTP responce
                        printf("%s",FILE_OPEN_ERR);
                      }
                    }
                    else
                    {
                      fclose(fp);
                      printf("%s",SYSTEM_ERR);
                    }
                  }
                  else
                  {
                    //Send failure HTTP responce
                    printf("%s",FILE_OPEN_ERR);
                  }
                }
                else
                {
                  printf("%s%d\n\n{\"result\":\"1\", \"ip\":\"%s\"}",HTML_HEADER,(int)(23+strlen(linfo->ip)),linfo->ip);
                }
              }
              else
              {
                //Close File and send failure HTTP response
                fclose(fp);
                printf("%s",FILE_READ_ERR);
              }
            }
            else
            {
              //Send password mismatch HTTP response
              printf("%s",LOGIN_FAIL);
            }
          }
        }
        else
        {
          //Close File and send failure HTTP responce
          fclose(fp);
          printf("%s",FILE_READ_ERR);
        }
      }

    }
    else
    {
      //Send HTTP responce reporting broken message
      printf("%s",GOT_NOTHING);
    }
  }
  else
  {
    //Send HTTP responce reporting broken message
    printf("%s",GOT_NOTHING);
  }
  return 0;
}
/*===========================================================================
  FUNCTION http_parser
===========================================================================*/
/*!
@brief
  This function parses data fetched from HTTP post message.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void http_parser(char *http_cgi_buff,
                 char http_element_name[][MAX_ELEMENT_LENGTH],
                 char http_element_val[][MAX_ELEMENT_LENGTH],
                 int elements)
{
  int i = 0, j = 0, k = 0;
  int len = 0;

  len = strlen(http_cgi_buff);
  i = 0;
  j = 0;
  if (len > 0)
  {
    //Parse till end of string and till it reaches max. element count
    while((k < len) && (i < elements))
    {
      j = 0;
      //Read till we reach delimiter to seperate value and element
      while((http_cgi_buff[k] != '=') && (k < len) && (j < MAX_ELEMENT_LENGTH))
      {
        http_element_name[i][j] = http_cgi_buff[k];
        k++;
        j++;
      }
      j = 0;
      k++;
      //Read till we reach delimiter to seperate modules
      while((http_cgi_buff[k] != '&') && (k < len) && (j < MAX_ELEMENT_LENGTH))
      {
        http_element_val[i][j] = http_cgi_buff[k];
        k++;
        j++;
      }
      k++;
      i++;
    }
  }
}
/*===========================================================================
  FUNCTION unencode_string
===========================================================================*/
/*!
@brief
  parses HTML code.

@input
  values             - cgi form field values.

@return
 1  - success
 -1 - failure

@dependencies
  None

@sideefects
  None
*/
/*=========================================================================*/
int unencode_string
(
  char *src,
  int last
)
{
  char* tmp=NULL;
  char hexs[3];
  int i=0,j=0;
  int ascii=0;
   tmp=(char*) malloc(last);
   if(tmp != NULL)
   {
     for(i=0,j=0; ( (i <= last ) && (src[i] != NULL) );)
     {
       //look for % special char which indicates html chars needs to be url encoded!!
       if( (src[i] == '%') )
       {
         //take next 2 char's into a hex string
         hexs[0]= src[i+1];
         hexs[1]= src[i+2];
         hexs[2]= '\0';
         //convert it into int from hex string.
         ascii= (int)strtol(hexs, NULL, 16);
         //convert int into character and store it. this is actual char.
         tmp[j]=(char) ascii;
         i += 3;
         j++;
       }
       else
       {
         //for other chars store as it is.
         tmp[j]=src[i];
         i++;
         j++;
       }
     }
     tmp[j]='\0';
     //copy transalated string into source string so that called fun can utilize.
     memcpy(src, tmp, strlen(tmp)+1);
     //free memory
     free(tmp);
     //set null to pointer.
     tmp=NULL;
     return SUCCESS;
   }
   else
   {
     return FAIL;
   }
 }
