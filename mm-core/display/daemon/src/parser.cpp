/*
 * DESCRIPTION
 * This file creates the parser for postprocessing features.
 * The parser parses the OEM config file on target.
 *
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 */

#include "parser.h"

int32_t adp_table::readTable(FILE* fp) {
    char line[1024];
    if (!readLabel(fp))
        return -1;
    if (!readSize(fp))
        return -1;
    data = (char *)calloc(tableSize + 1, sizeof(char));
    if ((this->data) != NULL) {
        fread(data, sizeof(char), tableSize, fp);
        data[tableSize] = '\0';
    }
    //discard other contents on this line
    fgets(line, sizeof(line), fp);
    return 0;
}

void adp_table::dumpTable() {
    dumpLabel();
    dumpSize();
    LOGE("Data: %s", data);
}

int32_t adp_table::getTable(float (&table)[CC_MATRIX_DIM][CC_MATRIX_DIM]) {
    if (!data) {
        LOGE("No data in table");
        return -1;
    }
    char *tmp = (char *)calloc(tableSize + 1, sizeof(char));
    char *tmp_r;
    if (tmp == NULL) {
        LOGE("Failed: Memory allocation failure");
        return -1;
    }
    memcpy(tmp, data, tableSize + 1);
    tmp[tableSize] = '\0';
    char *val = strtok_r (tmp, " ,\t\n", &tmp_r);
    int row = 0, col = 0;
    const int cc_row_max = 3, cc_col_max = 2;
    while (NULL != val) {
        table[row][col] = atoi(val);
        if(++col > cc_col_max) {
            if (++row > cc_row_max) {
                LOGE("Data overflow while reading table!");
                free(tmp);
                return -1;
            }
            col = 0;
        }
        val = strtok_r (NULL, ",", &tmp_r);
    }
    table[cc_row_max][cc_row_max] = 1;
    free(tmp);
    return 0;
}

int32_t adp_feature::readFeature(FILE *fp) {
    if (!readLabel(fp))
        return -1;
    if (!readTableCount(fp))
        return -1;
    if(!readDefaultTable(fp))
        return -1;
    tables = (adp_table *)calloc(tableCount, sizeof(adp_table));
    for (int i=0; i < tableCount; i++) {
        if (-1 == tables[i].readTable(fp))
            return -1;
    }
    if (!readChecksum(fp))
        return -1;
    return 0;
}

void adp_feature::dumpFeature() {
    dumpLabel();
    dumpTableCount();
    dumpDefaultTable();
    for (int i=0; i < tableCount; i++) {
        LOGE("Table %d", i);
        tables[i].dumpTable();
    }
    dumpChecksum();
}

bool adp_feature::verifyChecksum() {
    /* calculate and compare the CRC32 checksum */
    unsigned long calcChecksum = crc32(0L, Z_NULL, 0);
    calcChecksum = crc32(calcChecksum, (const Bytef*)this, sizeof(adp_feature));
    for (int i=0; i<tableCount; i++) {
        calcChecksum = crc32(calcChecksum, (const Bytef*)&tables[i],
                sizeof(adp_table));
        calcChecksum = crc32(calcChecksum, (const Bytef*)tables[i].getData(),
                tables[i].getSize());
    }
    if (calcChecksum == checksum) {
        valid = true;
    } else
        LOGE("Failed: feature checksum does not match!");
    return true;
}

int32_t adp_global::init(FILE *fp) {
    if (NULL == fp)
        return -1;
    if (!readHeader(fp))
        return -1;
    if (!readVersion(fp))
        return -1;
    if (!readFeatureCount(fp))
        return -1;
    features = (adp_feature *)calloc(featureCount, sizeof(adp_feature));
    for (int i=0; i<featureCount; i++) {
        if (-1 == features[i].readFeature(fp))
            return -1;
    }
    if (!readChecksum(fp))
        return -1;
    return 0;
}

void adp_global::dump() {
    dumpHeader();
    dumpVersion();
    dumpFeatureCount();
    for (int i=0; i<featureCount; i++) {
        LOGE("Feature %d:", i);
        features[i].dumpFeature();
    }
    dumpChecksum();
}

bool adp_global::verifyChecksum(FILE *fp) {
    /*calculate and compare the CRC32 checksum */
    for (int i=0; i<featureCount; i++) {
        features[i].verifyChecksum();
    }
    if (!fp)
        return false;
    unsigned long calcChecksum = crc32(0L, Z_NULL, 0);
    fseek(fp, 0, SEEK_SET);
    char *buffer = (char *)calloc(1024, sizeof(char));
    if (buffer == NULL) {
       LOGE("Failed: Memory allocation failure");
       return false;
     }

    while(!feof(fp)) {
        int len = fread(buffer, 1024, sizeof(char), fp);
        calcChecksum = crc32(calcChecksum, (const Bytef*)buffer, len);
    }
    if (calcChecksum == checksum) {
        valid = true;
    } else
        LOGE("Failed: file checksum does not match!");
    free(buffer);
    return true;
}

int32_t adp_global::getDCCNumberOfProfiles() {
    if (!valid)
        return 0;
    for (int i=0; i<featureCount; i++) {
        if (!strncmp(features[i].getLabel(), "CSC", 3)
                && features[i].isValid()) {
            return features[i].getTableCount();
        }
    }
    return 0;
}

int32_t adp_global::getDCCProfiles(char *list, uint32_t size) {
    if (!valid)
        return -1;
    uint32_t listLength = (featureCount * LABEL_SIZE) + featureCount;
    if (listLength > size) {
        printf("Insufficient storage for profile list");
        return -1;
    }

    for (int i=0; i<featureCount; i++) {
        if (!strncmp(features[i].getLabel(), "CSC", 3)
                && features[i].isValid()) {
            for (int j=0; j<features[i].getTableCount(); j++) {
                memcpy(list, features[i].tables[j].getLabel(), LABEL_SIZE);
                list += LABEL_SIZE;
                *list++ = ';';
            }
            *list++ = '\0';
        }
    }
    return 0;
}

int32_t adp_global::setDCCProfile(const char* profile) {
    if(!valid)
        return -1;
    display_pp_conv_cfg usrCfg;
    memset(&usrCfg, 0, sizeof(usrCfg));
    usrCfg.ops = 1;
    int32_t err = 0;
    bool found = false;
    /* get the user selected table values */
    for (int i=0; i<featureCount; i++) {
        if (!strncmp(features[i].getLabel(), "CSC", 3)
                && features[i].isValid()) {
            for (int j=0; j<features[i].getTableCount(); j++) {
                if (!strncmp(features[i].tables[j].getLabel(), profile,
                        strlen(profile))) {
                    err = features[i].tables[j].getTable(usrCfg.cc_matrix);
                    found = true;
                    break;
                }
            }
        }
    }
    if (!found) {
        LOGE("Failed to get the profile values");
        return -1;
    }
    /* set the values */
    err = display_pp_conv_set_cfg(MDP_BLOCK_DMA_P, &usrCfg);
    if(err) {
        LOGE("Failed to set hsic values");
        return err;
    }
    return 0;
}
