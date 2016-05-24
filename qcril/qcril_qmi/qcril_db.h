
#ifndef QCRIL_DB_H
#define QCRIL_DB_H

#include <stdio.h>
#include <sqlite3.h>

#include "qcril_log.h"
#include "qcril_qmi_nas.h"


#define QCRIL_MAX_EMERGENCY_NUMBERS_LEN 200
#define QCRIL_MAX_IMS_ADDRESS_LEN 81

int qcril_db_init(void);

/*===========================================================================

  FUNCTION  qcril_db_is_mcc_part_of_emergency_numbers_table

===========================================================================*/
/*!
    @brief
    Checks for mcc existence in db and retireves emergency number from db.

    @return
    0 if function is successful.
*/
/*=========================================================================*/
int qcril_db_is_mcc_part_of_emergency_numbers_table
(
    qmi_ril_custom_emergency_numbers_source_type source,
    char *mcc,
    char *emergency_num
);

/*===========================================================================

  FUNCTION  qcril_db_is_mcc_part_of_emergency_numbers_table_with_service_state

===========================================================================*/
/*!
    @brief
    Checks for mcc & service state existence in db and retrives
    emergency number from db.

    @return
    0 if function is successful.
*/
/*=========================================================================*/
int qcril_db_is_mcc_part_of_emergency_numbers_table_with_service_state
(
    qmi_ril_custom_emergency_numbers_source_type source,
    char *mcc,
    char *service_state,
    char *emergency_num
);

/*===========================================================================

  FUNCTION  qcril_db_is_number_mcc_part_of_emergency_numbers_table

===========================================================================*/
/*!
    @brief
    Checks for mcc and number existence in db

    @return
    0 if function is successful.
*/
/*=========================================================================*/
int qcril_db_is_number_mcc_part_of_emergency_numbers_table
(
    char *emergency_num,
    char *mcc,
    qmi_ril_custom_emergency_numbers_source_type source
);

/*===========================================================================

  FUNCTION  qcril_db_is_ims_address_for_mcc_emergency_number_part_of_emergency_numbers_table

===========================================================================*/
/*!
    @brief
    Checks for mcc and emergency number existence in db and retrieves
    corresponding ims_address (if present) from db.

    @return
    TRUE if function is successful.
*/
/*=========================================================================*/
int qcril_db_is_ims_address_for_mcc_emergency_number_part_of_emergency_numbers_table
(
    qmi_ril_custom_emergency_numbers_source_type source,
    char *mcc,
    char *emergency_num,
    char *ims_address
);


/*===========================================================================

  FUNCTION  qcril_db_query_escv_type

===========================================================================*/
/*!
    @brief
    Query ESCV type nased upon iin or (mcc, mnc)

    @return
    escv type
*/
/*=========================================================================*/
int qcril_db_query_escv_type
(
    char *emergency_num,
    char *iin,
    char *mcc,
    char *mnc,
    char *roam
);

#endif /* QCRIL_DB_H */
