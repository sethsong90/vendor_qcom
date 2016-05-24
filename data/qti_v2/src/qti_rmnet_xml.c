/******************************************************************************

                        QTI_RMNET_XML.C

******************************************************************************/

/******************************************************************************

  @file    qti_rmnet_xml.c
  @brief   Qualcomm Tethering Interface for RMNET tethering. This file
           implements the rmnet_config.txt parsing functionality.

  ---------------------------------------------------------------------------
  Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------


******************************************************************************/


/******************************************************************************

                      EDIT HISTORY FOR FILE

when       who        what, where, why
--------   ---        -------------------------------------------------------
3/6/13   sb         Initial version

******************************************************************************/


/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <arpa/inet.h>
#include "xmllib_common.h"
#include "xmllib_parser.h"
#include "ds_util.h"
#include "ds_Utils_DebugMsg.h"
#include "qti.h"

/*===========================================================================
                          DEFINITIONS
============================================================================*/

/*---------------------------------------------------------------------------
 Max allowed size of the XML file (2 MB)
---------------------------------------------------------------------------*/
#define QTI_XML_MAX_FILESIZE       (2 << 20)

/*-------------------------------------------------------------------------
 Defines for clipping space or space & quotes (single, double)
--------------------------------------------------------------------------*/
#define QTI_XML_CLIP_SPACE         " "
#define QTI_XML_CLIP_SPACE_QUOTES  " '\""

#define MAX_XML_STR_LEN                 120

/*-------------------------------------------------------------------------
  All the XML TAG
-------------------------------------------------------------------------*/
#define config_TAG                 "config"
#define link_prot_TAG              "link_prot"
#define ul_tlp_TAG                 "ul_tlp"
#define autoconnect_TAG            "autoconnect"
#define dl_data_agg_protocol_TAG   "dl_data_agg_protocol"
#define IP_TAG                     "IP"
#define ETH_TAG                    "ETH"
#define enabled_TAG                "enabled"
#define disabled_TAG               "disabled"
#define dl_tlp_enabled_TAG         "dl_tlp_enabled"
#define dl_qc_ncm_enabled_TAG      "dl_qc_ncm_enabled"
#define ul_qc_ncm_enabled_TAG      "ul_qc_ncm_enabled"
#define ul_mbim_enabled_TAG        "ul_mbim_enabled"
#define dl_mbim_enabled_TAG        "dl_mbim_enabled"

/*-----------------------------------------------------------------------
 Structure for storing the xml data from file, current read position and
   size information
------------------------------------------------------------------------*/
struct xml_data
{
   uint32 size;  /* Actual size of the file data */
   uint32 read;  /* Current read position in buff */
   char *buff;   /* Buffer containing the xml file contents */
};


/*----------------------------------------------------------------------
 Private function declarations
-----------------------------------------------------------------------*/

static int32 qti_xml_peekbytes_cb(
   struct xmllib_metainfo_s_type *metainfo,
   int32                          offset,
   int32                          bytecount,
   uint8                         *buffer
);

static int32 qti_xml_getbytes_cb(
   struct xmllib_metainfo_s_type *metainfo,
   int32                          bytecount,
   char                          *buffer
);

static int qti_xml_read_xml_file(
   const char *xm_file,
   struct xml_data *xml
);

static int qti_xml_parse_tree(
   xmllib_parsetree_node_s_type *xml_root,
   qti_rmnet_autoconnect_config *config
);

/*===========================================================================
                         FUNCTION DEFINITION
============================================================================*/

/*===========================================================================

FUNCTION QTI_UTIL_ICMP_STRING()

DESCRIPTION

  This function returns the result of case insensitive comparison of a
  xmllib's string (xml_str) and a regular string (str)

DEPENDENCIES
  None.

RETURN VALUE
  0 - if both strings are equal
  1 - First string has greater value
  -1 - Second string has greater value

SIDE EFFECTS
  None

/*=========================================================================*/
static int32 qti_util_icmp_string
(
   const xmllib_string_s_type *xml_str,
   const char *str
)
{
   int32 ret = -1;

   if (NULL != xml_str && NULL != str)
   {
      uint32 len = strlen(str);

      /* If the lengths match, do the string comparison */
      if (xml_str->len == len)
      {
         ret = strncasecmp(xml_str->string, str, len);
      }
   }

   return ret;
}

/*===========================================================================

FUNCTION QTI_WRITE_XML()

DESCRIPTION

  This function
  - writes the default rmnet autoconnect config

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/
int qti_write_xml(char *xml_file, qti_rmnet_autoconnect_config *config)
{
   FILE *fp = NULL;
   int i;
   struct in_addr addr;

   fp = fopen(xml_file, "wb");

   /* File not found at the given path */
   if (NULL == fp)
   {
      return QTI_FAILURE;
   }

   fprintf(fp, "<config>\n");
   fprintf(fp, "\t<autoconnect>disabled</autoconnect>\n");
   fprintf(fp, "\t<link_prot>ETH</link_prot>\n");
   fprintf(fp, "\t<ul_tlp>disabled</ul_tlp>\n");
   fprintf(fp, "\t<dl_data_agg_protocol>disabled</dl_data_agg_protocol>\n");
   fprintf(fp, "</config>\n");

   fclose(fp);

   return QTI_SUCCESS;
}


/*===========================================================================

FUNCTION QTI_READ_XML()

DESCRIPTION

  This function
  - reads the rmnet autoconnect config parameters

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/
int qti_read_xml(char *xml_file, qti_rmnet_autoconnect_config *config)
{
   xmllib_parsetree_node_s_type *xml_root = NULL;
   struct xml_data xml_data = {0, 0, NULL};

   /* Update the xml_data structure with data from the xml file */
   int32 ret_val = qti_xml_read_xml_file(xml_file, &xml_data);

   if (QTI_SUCCESS == ret_val)
   {
      xmllib_error_e_type error = 0;
      int32 xmllib_ret;
      struct xmllib_metainfo_s_type metainfo = {
                                                  &xml_data,
                                                  (xmllib_memalloc_fptr_type)malloc,
                                                  free,
                                                  qti_xml_peekbytes_cb,
                                                  qti_xml_getbytes_cb
                                               };

      /* Invoke the XML parser and obtain the parse tree */
      xmllib_ret = xmllib_parser_parse(XMLLIB_ENCODING_UTF8,
                                       &metainfo,
                                       &xml_root,
                                       &error);

      if (XMLLIB_SUCCESS == xmllib_ret)
      {
         ret_val = QTI_SUCCESS;
      }
      else
      {
         LOG_MSG_ERROR("qti_xml_parse: xmllib returned parse error", 0, 0, 0);
         ret_val = QTI_FAILURE;
      }
   }

   if (ret_val != QTI_SUCCESS)
   {
     if (xml_data.buff)
       free(xml_data.buff);

     return ret_val;
   }

   if (QTI_SUCCESS == ret_val)
   {
      /* parse the xml tree returned by the xmllib */
      ret_val = qti_xml_parse_tree(xml_root, config);

      if (ret_val != QTI_SUCCESS)
         LOG_MSG_ERROR("qti_xml_parse: qti_xml_parse_tree returned parse error", 0, 0, 0);

      /* Free up the xmllib's parse tree */
      xmllib_parser_free(free, xml_root);
   }

   /* Free the buffer allocated by the xml file reading utility function */
   if (xml_data.buff)
     free(xml_data.buff);

   return ret_val;

}

/*===========================================================================

FUNCTION QTI_XML_READ_XML_FILE()

DESCRIPTION

  This function is reads the given XML file and stores the contents in
  the given xml_data structure

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/
static int qti_xml_read_xml_file
(
   const char *file_name,
   struct xml_data *xml_data
)
{
   FILE *fp = NULL;
   int32 ret_val = QTI_FAILURE;
   int32 cur_pos;

   fp = fopen(file_name, "rb");

   /* File not found at the given path */
   if (NULL == fp)
   {
      LOG_MSG_ERROR("qti_xml_read_xml_file: unable to open file ",
                    0, 0, 0);
      ret_val = QTI_FAILURE;
   }
   /* If seek to the end failed or file size is greater than what we support */
   else if (fseek(fp, 0, SEEK_END) ||
            ((cur_pos = ftell(fp)) < 0 || cur_pos > QTI_XML_MAX_FILESIZE) )
   {
      fclose(fp);
   }
   else
   {
      xml_data->size = cur_pos;

      /* Allocate storage for reading the xml file into memory */
      if (NULL == (xml_data->buff = malloc(xml_data->size)))
      {
         LOG_MSG_ERROR("qti_xml_read_xml_file: failed to allocate "
                       "memory for read buffer", 0, 0, 0);
         ret_val = QTI_FAILURE;
      }
      /* Reset to the beginning of the file */
      else if (!fseek(fp, 0, SEEK_SET))
      {
         size_t read_size;

         /* Read the data from file to buffer */
         read_size = fread(xml_data->buff, 1, xml_data->size, fp);

         if (!ferror(fp) && (read_size == xml_data->size))
         {
            xml_data->read = 0;
            ret_val = QTI_SUCCESS;
         }
      }

      fclose(fp);
   }
   return ret_val;
}

/*===========================================================================

FUNCTION QTI_XML_PEEKBYTES_CB()

DESCRIPTION

  This function
  - facilitates xml read

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/

static int32 qti_xml_peekbytes_cb
(
  struct xmllib_metainfo_s_type *metainfo,
  int32                          offset,
  int32                          bytecount,
  uint8                         *buffer
)
{
   struct xml_data *xml_data = NULL;

   /* Validate arguments */
   if ((NULL == metainfo)                       ||
       (NULL == (xml_data = metainfo->xmltext)) ||
       (offset < 0)                             ||
       (bytecount < 0)                          ||
       (NULL == buffer)                         ||
       (xml_data->read+offset+bytecount > xml_data->size))
   {
      return XMLLIB_ERROR;
   }

   memcpy(buffer, xml_data->buff+xml_data->read+offset, bytecount);
   return XMLLIB_SUCCESS;
}

/*===========================================================================

FUNCTION QTI_XML_GETBYTES_CB()

DESCRIPTION

  This function
  - facilitates xml read

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/

static int32 qti_xml_getbytes_cb
(
  struct xmllib_metainfo_s_type *metainfo,
  int32                          bytecount,
  char                          *buffer
)
{
   struct xml_data *xml_data = NULL;

   /* If requesting to read more than what we have return error */
   if ((NULL == metainfo)                       ||
       (NULL == (xml_data = metainfo->xmltext)) ||
       (bytecount < 0)                          ||
       (xml_data->read+bytecount > xml_data->size))
   {
      return XMLLIB_ERROR;
   }

   /* If a valid buffer is given, copy the data */
   if (NULL != buffer)
   {
      memcpy(buffer, xml_data->buff+xml_data->read, bytecount);
   }

   /* Increment to the next unread data block */
   xml_data->read += bytecount;

   return XMLLIB_SUCCESS;
}

/*===========================================================================

FUNCTION QTI_READ_CONTENT_ELEMENT()

DESCRIPTION

  This function
  - facilitates xml read

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/
static xmllib_string_s_type* qti_read_content_element
(
 xmllib_parsetree_node_s_type* element
)
{
  xmllib_parsetree_node_s_type* child_ptr;

  for ( child_ptr  = element->payload.element.child;
        child_ptr != 0;
        child_ptr  = child_ptr->sibling )
  {
    if ( child_ptr->nodetype == XMLLIB_PARSETREE_NODE_CONTENT )
    {
      return &(child_ptr->payload.content);
    }
  }

  return ((xmllib_string_s_type*)NULL);
}

/*===========================================================================

FUNCTION QTI_XML_PARSE_TREE()

DESCRIPTION

  This function
  - parses through the xml file

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/

static int qti_xml_parse_tree
(
   xmllib_parsetree_node_s_type *xml_node,
   qti_rmnet_autoconnect_config *config
)
{
   static int ssid = 0, dhcp = 0;
   int32 ret_val = QTI_SUCCESS;
   xmllib_string_s_type* content;
   char content_buf[MAX_XML_STR_LEN];

   if (NULL == xml_node)
      return ret_val;

   while ( (xml_node != (xmllib_parsetree_node_s_type *)NULL) &&
          (ret_val == QTI_SUCCESS) )
   {
     switch (xml_node->nodetype)
     {
       case XMLLIB_PARSETREE_NODE_ELEMENT:
       {
         if (0 == qti_util_icmp_string(&xml_node->payload.element.name,
                                         config_TAG)
             )
         {
           // go to child
           ret_val = qti_xml_parse_tree(xml_node->payload.element.child,
                                                   config);
         }
         else if (0 == qti_util_icmp_string(&xml_node->payload.element.name,
                  autoconnect_TAG))
         {
           content = qti_read_content_element(xml_node);
           if (content)
           {
             memset(content_buf, 0, sizeof(content_buf));
             memcpy(content_buf, (void *)content->string, content->len);
             if (strncasecmp(enabled_TAG, content_buf, strlen(enabled_TAG)) == 0)
             {
               config->auto_connect = TRUE;
             }
             else if (strncasecmp(disabled_TAG, content_buf, strlen(disabled_TAG)) == 0)
             {
               config->auto_connect = FALSE;
             }
             else
             {
               /* Default Value. */
               config->auto_connect = FALSE;
             }
           }
         }
         else if (0 == qti_util_icmp_string(&xml_node->payload.element.name,
                  link_prot_TAG))
         {
           content = qti_read_content_element(xml_node);
           if (content)
           {
             memset(content_buf, 0, sizeof(content_buf));
             memcpy(content_buf, (void *)content->string, content->len);
             if (strncasecmp(IP_TAG, content_buf, strlen(IP_TAG)) == 0)
             {
               config->link_prot = TETH_LINK_PROTOCOL_IP;
             }
             else if (strncasecmp(ETH_TAG, content_buf, strlen(ETH_TAG)) == 0)
             {
               config->link_prot = TETH_LINK_PROTOCOL_ETHERNET;
             }
             else
             {
               /* Default Value. */
                 config->link_prot = TETH_LINK_PROTOCOL_ETHERNET;
             }
           }
         }
          else if (0 == qti_util_icmp_string(&xml_node->payload.element.name,
                  ul_tlp_TAG))
         {
           content = qti_read_content_element(xml_node);
           if (content)
           {
             memset(content_buf, 0, sizeof(content_buf));
             memcpy(content_buf, (void *)content->string, content->len);
             if (strncasecmp(enabled_TAG, content_buf, strlen(enabled_TAG)) == 0)
             {
               config->ul_aggr_prot = TETH_AGGR_PROTOCOL_TLP;
             }
             else if (strncasecmp(disabled_TAG, content_buf, strlen(disabled_TAG)) == 0)
             {
               config->ul_aggr_prot = TETH_AGGR_PROTOCOL_NONE;
             }
             else if (strncasecmp(ul_qc_ncm_enabled_TAG, content_buf, strlen(disabled_TAG)) == 0)
             {
               config->ul_aggr_prot = TETH_AGGR_PROTOCOL_MBIM;
             }
             else if (strncasecmp(ul_mbim_enabled_TAG, content_buf, strlen(disabled_TAG)) == 0)
             {
               config->ul_aggr_prot = TETH_AGGR_PROTOCOL_MBIM;
             }
             else
             {
               /* Default Value. */
               config->ul_aggr_prot = TETH_AGGR_PROTOCOL_NONE;
             }
           }
         }
         else if (0 == qti_util_icmp_string(&xml_node->payload.element.name,
                  dl_data_agg_protocol_TAG))
         {
           content = qti_read_content_element(xml_node);
           if (content)
           {
             memset(content_buf, 0, sizeof(content_buf));
             memcpy(content_buf, (void *)content->string, content->len);
             if (strncasecmp(dl_tlp_enabled_TAG, content_buf, strlen(enabled_TAG)) == 0)
             {
               config->dl_aggr_prot = TETH_AGGR_PROTOCOL_TLP;
             }
             else if (strncasecmp(disabled_TAG, content_buf, strlen(disabled_TAG)) == 0)
             {
               config->dl_aggr_prot = TETH_AGGR_PROTOCOL_NONE;
             }
             else if (strncasecmp(dl_qc_ncm_enabled_TAG, content_buf, strlen(disabled_TAG)) == 0)
             {
               config->dl_aggr_prot = TETH_AGGR_PROTOCOL_MBIM;
             }
             else if (strncasecmp(dl_mbim_enabled_TAG, content_buf, strlen(disabled_TAG)) == 0)
             {
               config->dl_aggr_prot = TETH_AGGR_PROTOCOL_MBIM;
             }
             else
             {
               /* Default Value. */
                 config->dl_aggr_prot = TETH_AGGR_PROTOCOL_NONE;
             }
           }
         }
      }
      break;
      default:
      break;
    }
    // go to sibling
    xml_node = xml_node->sibling;
  } // while

  return ret_val;
}

