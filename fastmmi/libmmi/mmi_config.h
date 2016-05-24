/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __SYSTEM_CORE_MMI_CONFIG__
#define __SYSTEM_CORE_MMI_CONFIG__

#include <string>
#include <list>
#include <vector>
#include <hash_map>
#include <mmi_module_manage.h>
using namespace std;
typedef struct {
    char domain[64];
    char name[64];
    char value[1024];
} mmi_config_item_t;

enum {
	CONFIG_SUCCESS				= 0,
	CONFIG_NOT_FOUND_ERR		= 1,
	CONFIG_FORTMAT_ERR			= 2,
	CONFIG_TEST_CASE_ERR		= 3,
	CONFIG_NO_DEFAULT_CFG_ERR	= 4,
};
class mmi_config {
  public:
    static int load_config_from_file(const char *path, bool is_check);
    static const char *query_config_value(const char *domain, const char *name);
    static const char *query_config_value(const char *domain, const char *name, const char *def);
    static const char *get_config_domain_name(mmi_module * m);
    static const vector < string > get_config_domain_list();
    static void free_config();
  private:
    static void add_config_item(char *domain, char *name, char *value);
    static int parse_domain(const char *line, char *domain, int domainLen);
    static list < mmi_config_item_t * >config_list;
};
#endif
