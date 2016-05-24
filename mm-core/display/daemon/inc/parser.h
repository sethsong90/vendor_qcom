/*
 * Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef _PARSER_H
#define _PARSER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <cutils/log.h>
#include <linux/msm_mdp.h>

#include "lib-postproc.h"

#include "common_log.h"

#define LABEL_SIZE      20
#define MAX_LINESIZE    1024
#define ASCII_INT_SIZE  4
#define CC_MATRIX_DIM   4
#define NAME_SIZE       3
#define VERSION_SIZE    6

#ifndef LOGE_IF
#define LOGE_IF(cond, ...) \
    ( (CONDITION(cond)) ? ((void)LOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) : (void)0 )
#endif
#ifndef LOGD_IF
#define LOGD_IF(a...) while(0) {}
#endif

class adp_table {
private:
    char label[LABEL_SIZE];
    uint16_t tableSize;
    char *data;
    bool readLabel(FILE *fp) {
        char line[MAX_LINESIZE];
        if (LABEL_SIZE != fread(label, sizeof(char), LABEL_SIZE, fp))
            return false;
        //discard other contents on this line
        fgets(line, sizeof(line), fp);
        return true;
    }
    bool readSize(FILE *fp) {
        char line[MAX_LINESIZE];
        char ascii[ASCII_INT_SIZE];
        if (ASCII_INT_SIZE != fread(ascii, sizeof(char), ASCII_INT_SIZE, fp))
            return false;
        tableSize = atoi(ascii);
        //discard other contents on this line
        fgets(line, sizeof(line), fp);
        return true;
    }
    void dumpLabel() {
        LOGE("Lable: %s\n", label);
    }
    void dumpSize() {
        LOGE("Size: %d\n", tableSize);
    }
public:
    int32_t readTable(FILE *fp);
    void dumpTable();
    char *getLabel() { return label; }
    int32_t getTable(float (&table)[CC_MATRIX_DIM][CC_MATRIX_DIM]);
    char *getData() {
        return data;
    }
    int32_t getSize() {
        return tableSize;
    }
};

class adp_feature {
private:
    char label[3];
    uint16_t tableCount;
    uint32_t checksum;
    uint32_t defaultTable;
    bool valid;
    bool readLabel(FILE *fp) {
        char line[MAX_LINESIZE];
        if (NAME_SIZE != fread(label, sizeof(char), NAME_SIZE, fp))
            return false;
        //discard other contents on this line
        fgets(line, sizeof(line), fp);
        return true;
    }
    bool readTableCount(FILE *fp) {
        char line[MAX_LINESIZE];
        char ascii[ASCII_INT_SIZE];
        if (ASCII_INT_SIZE != fread(ascii, sizeof(char), ASCII_INT_SIZE, fp))
            return false;
        tableCount = atoi(ascii);
        //discard other contents on this line
        fgets(line, sizeof(line), fp);
        return true;
    }
    bool readDefaultTable(FILE *fp) {
        char line[MAX_LINESIZE];
        char ascii[ASCII_INT_SIZE];
        if (ASCII_INT_SIZE != fread(ascii, sizeof(char), ASCII_INT_SIZE, fp))
            return false;
        defaultTable = atoi(ascii);
        //discard other contents on this line
        fgets(line, sizeof(line), fp);
        return true;
    }
    bool readChecksum(FILE *fp) {
        char line[MAX_LINESIZE];
        if (sizeof(checksum) != fread(&checksum, sizeof(checksum), 1, fp))
            return false;
        //discard other contents on this line
        fgets(line, sizeof(line), fp);
        return true;
    }
    void dumpLabel() {
        LOGE("Label: %s\n", label);
    }
    void dumpTableCount() {
        LOGE("TableCount: %d\n", tableCount);
    }
    void dumpDefaultTable() {
        LOGE("Default Table: %d\n", defaultTable);
    }
    void dumpChecksum() {
        LOGE("Checksum: 0x%x\n", checksum);
    }
public:
    adp_table *tables;
    int32_t readFeature(FILE *fp);
    void dumpFeature();
    bool verifyChecksum();
    bool isValid() { return valid; }
    int32_t getTableCount() { return tableCount; }
    char *getLabel() { return label; }
};

class adp_global {
private:
    char header[20];
    char version[6];
    uint32_t checksum;
    uint16_t featureCount;
    adp_feature *features;
    bool valid;
    bool readHeader(FILE *fp) {
        char line[MAX_LINESIZE];
        if (LABEL_SIZE != fread(header, sizeof(char), LABEL_SIZE, fp))
            return false;
        //discard other contents on this line
        fgets(line, sizeof(line), fp);
        return true;
    }
    bool readVersion(FILE *fp) {
        char line[MAX_LINESIZE];
        if (VERSION_SIZE != fread(version, sizeof(char), VERSION_SIZE, fp))
            return false;
        //discard other contents on this line
        fgets(line, sizeof(line), fp);
        return true;
    }
    bool readFeatureCount(FILE *fp) {
        char line[MAX_LINESIZE];
        char ascii[ASCII_INT_SIZE];
        if (ASCII_INT_SIZE != fread(&ascii, sizeof(char), ASCII_INT_SIZE, fp))
            return false;
        featureCount = atoi(ascii);
        //discard other contents on this line
        fgets(line, sizeof(line), fp);
        return true;
    }
    bool readChecksum(FILE *fp) {
        if (sizeof(checksum) != fread(&checksum, sizeof(checksum), 1, fp))
            return false;
        return true;
    }
    void dumpHeader() {
        LOGE("Header: %s\n", header);
    }
    void dumpVersion() {
        LOGE("Version: %s\n", version);
    }
    void dumpFeatureCount() {
        LOGE("FeatureCount: %d\n", featureCount);
    }
    void dumpChecksum() {
        LOGE("Checksum: 0x%x\n", checksum);
    }
public:
    int32_t init(FILE *fp);
    void dump();
    bool verifyChecksum(FILE *fp);
    int32_t getDCCNumberOfProfiles();
    int32_t getDCCProfiles(char *list, uint32_t size);
    int32_t setDCCProfile(const char* profile);
};


#endif /* _PARSER_H */

