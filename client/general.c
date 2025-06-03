#include <stdlib.h>
#include "general.h"
#include "cJSON.h"


extern struct moniterdContext mctx[100];

UA_ByteString
loadFile(const char* const path) {
    UA_ByteString fileContents = UA_STRING_NULL;
    /* Open the file */
    FILE* fp = 0;
#if defined(_MSC_VER)
    fopen_s(&fp, path, "rb");
#else
    fp = fopen(path, "rb");
#endif
    if (!fp) {
        errno = 0; /* We read errno also from the tcp layer... */
        return fileContents;
    }

    /* Get the file length, allocate the data and read */
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte*)UA_malloc(fileContents.length * sizeof(UA_Byte));
    if (fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if (read != fileContents.length)
            UA_ByteString_clear(&fileContents);
    }
    else {
        fileContents.length = 0;
    }
    fclose(fp);
    return fileContents;
}

UA_StatusCode
UA_Client_readDataTypeDefinitionAttribute(UA_Client* client, const UA_NodeId nodeId, UA_StructureDefinition* outDataTypeDefinition) {
    return __UA_Client_readAttribute(client, &nodeId,
        UA_ATTRIBUTEID_DATATYPEDEFINITION,
        outDataTypeDefinition,
        &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION]);
}

char* general_strdup(char* str) {
    char* ret = malloc(strlen(str) + 1);
    memcpy(ret, str, strlen(str));
    ret[strlen(str)] = 0;
    return ret;
}

void buildCustomDataType(UA_Client* client, UA_NodeId nodeId) {
    char name[128];
    int devMemSize = 0;
    UA_StructureDefinition outDataType;
    UA_ClientConfig* cc = UA_Client_getConfig(client);
    UA_StatusCode ret = UA_Client_readDataTypeDefinitionAttribute(client, nodeId, &outDataType);
    if (ret != UA_STATUSCODE_GOOD) {
        printf("Can't find DataTypeDefinition\n");
        return;
    }
    printf("-> Encoding %d:%d\n", outDataType.defaultEncodingId.namespaceIndex, outDataType.defaultEncodingId.identifier.numeric);
    printf("-> Structure %d\n", outDataType.structureType);

    UA_DataTypeMember* devMembers = (UA_DataTypeMember*)malloc(sizeof(UA_DataTypeMember) * outDataType.fieldsSize);
    for (int i = 0; i < outDataType.fieldsSize; i++) {
        memset(name, 0, sizeof(name));
        memcpy(name, outDataType.fields[i].name.data, outDataType.fields[i].name.length);
        devMembers[i].memberName = general_strdup(name);
        if (outDataType.fields[i].valueRank >= 1) {
            devMembers[i].isArray = true;
        }
        else {
            devMembers[i].isArray = false;
        }
        devMembers[i].padding = 0;
        devMembers[i].isOptional = outDataType.fields[i].isOptional;

        UA_DataType* dt = 0;
        for (size_t j = 0; j < UA_TYPES_COUNT; ++j) {
            if (UA_NodeId_order(&UA_TYPES[j].typeId, &outDataType.fields[i].dataType) == UA_ORDER_EQ) {
                dt = &UA_TYPES[j];
                break;
            }
        }
        if (dt == 0) {
            printf("Error can't find type\n");
            exit(-1);
        }
        devMembers[i].memberType = dt;
        if (devMembers[i].isArray) {
            devMemSize = devMemSize + sizeof(void*) + sizeof(size_t);
        }
        else {
            devMemSize = devMemSize + dt->memSize;
        }
        printf("field name : %s | type : %d:%d | array : %d\n", name, outDataType.fields[i].dataType.namespaceIndex, outDataType.fields[i].dataType.identifier.numeric, outDataType.fields[i].valueRank);
    }

    UA_DataType devType = {
        UA_TYPENAME(name)                /* .tyspeName */
        nodeId, /* .typeId */
        outDataType.defaultEncodingId, /* .binaryEncodingId, the numeric*/
        devMemSize,                      /* .memSize */
        UA_DATATYPEKIND_STRUCTURE,          /* .typeKind */
        false,                               /* .pointerFree */
        false,                              /* .overlayable (depends on endianness and the absence of padding) */
        outDataType.fieldsSize,                                  /* .membersSize */
        devMembers
    };

    UA_DataType* types = (UA_DataType*)malloc(sizeof(UA_DataType) * 1);
    types[0] = devType;

    UA_DataTypeArray tempcustomDataTypes = { NULL, 1, types, UA_FALSE };
    UA_DataTypeArray* customDataTypes = (UA_DataTypeArray*)malloc(sizeof(UA_DataTypeArray));//warn might memory leak;
    memcpy(customDataTypes, &tempcustomDataTypes, sizeof(UA_DataTypeArray));
    cc->customDataTypes = customDataTypes;
    return;
}

static unsigned char
printNumber(int n, char* pos, unsigned char min_digits) {
    char digits[10];
    unsigned char len = 0;
    /* Handle negative values */
    if (n < 0) {
        pos[len++] = '-';
        n = -n;
    }

    /* Extract the digits */
    unsigned char i = 0;
    for (; i < min_digits || n > 0; i++) {
        digits[i] = (char)((n % 10) + '0');
        n /= 10;
    }

    /* Print in reverse order and return */
    for (; i > 0; i--)
        pos[len++] = digits[i - 1];
    return len;
}

void dateTimeToString(UA_DateTime d, char* buffer) {
    UA_DateTimeStruct tSt = UA_DateTime_toStruct(d);
    char* pos = buffer;
    pos += printNumber(tSt.year, pos, 4);
    *(pos++) = '-';
    pos += printNumber(tSt.month, pos, 2);
    *(pos++) = '-';
    pos += printNumber(tSt.day, pos, 2);
    *(pos++) = 'T';
    pos += printNumber(tSt.hour, pos, 2);
    *(pos++) = ':';
    pos += printNumber(tSt.min, pos, 2);
    *(pos++) = ':';
    pos += printNumber(tSt.sec, pos, 2);
    *(pos++) = '.';
    pos += printNumber(tSt.milliSec, pos, 3);
    pos += printNumber(tSt.microSec, pos, 3);
    pos += printNumber(tSt.nanoSec, pos, 3);
    pos--;
    while (*pos == '0')
        pos--;
    if (*pos == '.')
        pos--;

    *(++pos) = 'Z';
    *(++pos) = 0;
    //UA_String str = { ((uintptr_t)pos - (uintptr_t)buffer) + 1, (UA_Byte*)buffer };
}

int extractValue(void* p, UA_DataType* dt, size_t arrayLength, char* key, cJSON* jsonParentNode) {
    if (!p) {
        cJSON_AddNullToObject(jsonParentNode, key);
        return 0;
    }

    double* item = (double*)malloc(sizeof(double) * (arrayLength + 1));//at least 1 tem
    memset(item, 0, sizeof(double) * (arrayLength + 1));
    int isScalar = ((arrayLength == 0) ? 1 : 0);

    if (arrayLength == 0) {
        arrayLength = 1;
    }
    if (dt->typeKind == UA_DATATYPEKIND_BOOLEAN) {
        for (int i = 0; i < arrayLength; i++) {
            item[i] = ((UA_Boolean*)p)[i];
        }
    }
    else if (dt->typeKind == UA_DATATYPEKIND_SBYTE) {
        for (int i = 0; i < arrayLength; i++) {
            item[i] = ((UA_SByte*)p)[i];
        }
    }
    else if (dt->typeKind == UA_DATATYPEKIND_BYTE) {
        for (int i = 0; i < arrayLength; i++) {
            item[i] = ((UA_Byte*)p)[i];
        }
    }
    else if (dt->typeKind == UA_DATATYPEKIND_INT16) {
        for (int i = 0; i < arrayLength; i++) {
            item[i] = ((UA_Int16*)p)[i];
        }
    }
    else if (dt->typeKind == UA_DATATYPEKIND_UINT16) {
        for (int i = 0; i < arrayLength; i++) {
            item[i] = ((UA_UInt16*)p)[i];
        }
    }
    else if (dt->typeKind == UA_DATATYPEKIND_INT32) {
        for (int i = 0; i < arrayLength; i++) {
            item[i] = ((UA_Int32*)p)[i];
        }
    }
    else if (dt->typeKind == UA_DATATYPEKIND_UINT32) {
        for (int i = 0; i < arrayLength; i++) {
            item[i] = ((UA_UInt32*)p)[i];
        }
    }
    else if (dt->typeKind == UA_DATATYPEKIND_INT64) {
        for (int i = 0; i < arrayLength; i++) {
            item[i] = (double)(((UA_Int64*)p)[i]);
        }
    }
    else if (dt->typeKind == UA_DATATYPEKIND_UINT64) {
        for (int i = 0; i < arrayLength; i++) {
            item[i] = (double)(((UA_UInt64*)p)[i]);
        }
    }
    else if (dt->typeKind == UA_DATATYPEKIND_FLOAT) {
        for (int i = 0; i < arrayLength; i++) {
            item[i] = ((UA_Float*)p)[i];
        }
    }
    else if (dt->typeKind == UA_DATATYPEKIND_DOUBLE) {
        for (int i = 0; i < arrayLength; i++) {
            item[i] = ((UA_Double*)p)[i];
        }
    }
    else if (dt->typeKind == UA_DATATYPEKIND_STRING) {
        //UA_String
        char temp[512];
        if (isScalar == 1) {
            memcpy(temp, ((UA_String*)p)[0].data, ((UA_String*)p)[0].length);
            temp[((UA_String*)p)[0].length] = 0;
            cJSON_AddStringToObject(jsonParentNode, key, temp);
        }
        else {
            cJSON* itemArray = cJSON_CreateArray();
            for (int i = 0; i < arrayLength; i++) {
                memcpy(temp, ((UA_String*)p)[i].data, ((UA_String*)p)[i].length);
                temp[((UA_String*)p)[i].length] = 0;
                cJSON_AddItemToArray(itemArray, cJSON_CreateString(temp));
            }
            cJSON_AddItemToObject(jsonParentNode, key, itemArray);
        }
        free(item);
        return 0;
    }
    else if (dt->typeKind == UA_DATATYPEKIND_DATETIME) {
        char buffer[128];
        if (isScalar == 1) {
            dateTimeToString(((UA_DateTime*)p)[0], buffer);
            cJSON_AddStringToObject(jsonParentNode, key, buffer);
        }
        else {
            cJSON* itemArray = cJSON_CreateArray();
            for (int i = 0; i < arrayLength; i++) {
                dateTimeToString(((UA_DateTime*)p)[0], buffer);
                cJSON_AddItemToArray(itemArray, cJSON_CreateString(buffer));
            }
            cJSON_AddItemToObject(jsonParentNode, key, itemArray);
        }
        free(item);
        return 0;
    }
    else if (dt->typeKind == UA_DATATYPEKIND_LOCALIZEDTEXT) {
        char temp_locale[128];
        char temp_text[128];
        if (isScalar == 1) {
            cJSON* localeObj = cJSON_CreateObject();
            memcpy(temp_locale, ((UA_LocalizedText*)p)[0].locale.data, ((UA_LocalizedText*)p)[0].locale.length);
            memcpy(temp_text, ((UA_LocalizedText*)p)[0].text.data, ((UA_LocalizedText*)p)[0].text.length);
            temp_locale[((UA_LocalizedText*)p)[0].locale.length] = 0;
            temp_text[((UA_LocalizedText*)p)[0].text.length] = 0;
            cJSON_AddStringToObject(localeObj,"locale", temp_locale);
            cJSON_AddStringToObject(localeObj,"text", temp_text);
            cJSON_AddItemToObject(jsonParentNode, key, localeObj);
        }
        else {
            cJSON* itemArray = cJSON_CreateArray();
            for (int i = 0; i < arrayLength; i++) {
                cJSON* localeObj = cJSON_CreateObject();
                memcpy(temp_locale, ((UA_LocalizedText*)p)[i].locale.data, ((UA_LocalizedText*)p)[i].locale.length);
                memcpy(temp_text, ((UA_LocalizedText*)p)[i].text.data, ((UA_LocalizedText*)p)[i].text.length);
                temp_locale[((UA_LocalizedText*)p)[i].locale.length] = 0;
                temp_text[((UA_LocalizedText*)p)[i].text.length] = 0;
                cJSON_AddStringToObject(localeObj, "locale", temp_locale);
                cJSON_AddStringToObject(localeObj, "text", temp_text);
                cJSON_AddItemToArray(itemArray, localeObj);
            }
            cJSON_AddItemToObject(jsonParentNode, key, itemArray);
        }
        free(item);
        return 0;
    }
    else {
        printf("unsupport type %d @extractValue\n", dt->typeKind);
        free(item);
        return -1;
    }
    if (isScalar == 1) {//scalar
        cJSON_AddNumberToObject(jsonParentNode, key, item[0]);
    }
    else {
        cJSON* itemArray = cJSON_CreateArray();
        for (int i = 0; i < arrayLength; i++) {
            cJSON_AddItemToArray(itemArray, cJSON_CreateNumber(item[i]));
        }
        cJSON_AddItemToObject(jsonParentNode, key, itemArray);
    }
    free(item);
    return 0;
}

void extractComplexDataType(void* pData, const UA_DataType* type,char * key, cJSON* jsonParentNode) {
    UA_DataTypeMember* dtm = type->members;
    UA_UInt32 dtmSize = type->membersSize;
    cJSON* jsonValueNode = cJSON_CreateObject();
    for (UA_UInt32 i = 0; i < dtmSize; i++) {
        int isArray = dtm[i].isArray;
        pData = ((char*)pData) + dtm[i].padding;
        UA_UInt32 type = dtm[i].memberType->typeKind;
        if (isArray) {
            size_t arrayLength = *((size_t*)pData);
            printf("arraySize : %ld | dataTypeKind : %d\n", (long)arrayLength, type);
            pData = ((char*)pData) + sizeof(size_t);
            void* arrayPtr = *((void**)(pData));
            printf("arrayPtr : %p\n", arrayPtr);
            if (arrayLength == 0) {
                arrayPtr = 0;
            }
            if (dtm[i].memberType->membersSize > 0) {
                printf("untest condition!!!!\n");
                extractComplexDataTypeArray(arrayPtr, dtm[i].memberType, arrayLength, (char*)dtm[i].memberName, jsonValueNode);
            }
            else {
                extractValue(arrayPtr, (UA_DataType*)dtm[i].memberType, arrayLength, (char*)dtm[i].memberName, jsonValueNode);
            }
            pData = ((char*)pData) + sizeof(void*);
        }
        else {
            printf("scalar | dataKind : %d\n", type);
            if (dtm[i].memberType->membersSize > 0) {
                extractComplexDataType(pData, dtm[i].memberType, (char*)dtm[i].memberName, jsonValueNode);
            }
            else {
                extractValue(pData, (UA_DataType*)dtm[i].memberType, 0, (char*)dtm[i].memberName, jsonValueNode);
            }
            pData = ((char*)pData) + dtm[i].memberType->memSize;
        }
    }
    
    if (jsonParentNode->type == cJSON_Array) {
        cJSON_AddItemToArray(jsonParentNode, jsonValueNode);
    }
    else {
        cJSON_AddItemToObject(jsonParentNode, key, jsonValueNode);
    }
    return ;
}

void extractComplexDataTypeArray(void* pData, const UA_DataType* type , size_t arrayLength , char* key, cJSON* jsonParentNode) {
    cJSON* jsonValueNodeArray = cJSON_CreateArray();
    for (int k = 0; k < arrayLength; k++) {
        extractComplexDataType(pData, type, "", jsonValueNodeArray);
        pData = ((char*)pData) + type->memSize;
    }
    cJSON_AddItemToObject(jsonParentNode, "value", jsonValueNodeArray);
}

void extractDataType(UA_Variant* value, cJSON* jsonParentNode) {
    UA_DataTypeMember* dtm = value->type->members;
    UA_UInt32 dtmSize = value->type->membersSize;
    void* pData = value->data;
    if (dtmSize == 0) {
        extractValue(value->data, (UA_DataType*)value->type, value->arrayLength, "value", jsonParentNode);
        return;
    }

    if (value->arrayLength == 0) {//scalar
        extractComplexDataType(pData, value->type,"value", jsonParentNode);
        pData = ((char*)pData) + value->type->memSize;
        
    } else {
        extractComplexDataTypeArray(pData, value->type,value->arrayLength, "value", jsonParentNode);
        pData = ((char*)pData) + (value->type->memSize * value->arrayLength);
    }
}

cJSON* nodeIdToJson(UA_NodeId nodeId) {
    char tempstr[128];
    memset(tempstr, 0, sizeof(tempstr));
    cJSON* jsonNodeId = cJSON_CreateObject();
    cJSON_AddNumberToObject(jsonNodeId, "namespaceindex", nodeId.namespaceIndex);
    cJSON_AddNumberToObject(jsonNodeId, "identifiertype", nodeId.identifierType);
    if (nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
        cJSON_AddNumberToObject(jsonNodeId, "identifier", nodeId.identifier.numeric);
    }

    if (nodeId.identifierType == UA_NODEIDTYPE_STRING) {
        memcpy(tempstr, nodeId.identifier.string.data, nodeId.identifier.string.length);
        cJSON_AddStringToObject(jsonNodeId, "identifier", tempstr);
    }
    if (nodeId.identifierType == UA_NODEIDTYPE_BYTESTRING) {
        memcpy(tempstr, nodeId.identifier.byteString.data, nodeId.identifier.byteString.length);
        cJSON_AddStringToObject(jsonNodeId, "identifier", tempstr);
    }
    if (nodeId.identifierType == UA_NODEIDTYPE_GUID) {
        UA_Guid_to_hex(&nodeId.identifier.guid, tempstr, true);
        cJSON* guid = cJSON_CreateObject();
        cJSON_AddStringToObject(jsonNodeId, "identifier", tempstr);
    }

    return jsonNodeId;
}

void retriveBasicAttribute(UA_Client* client, UA_NodeId nodeId, cJSON* root) {
    char tempStr[256];
    UA_QualifiedName bn;
    UA_LocalizedText dn;
    UA_LocalizedText desc;
    UA_UInt32 wm;
    UA_UInt32 uwm;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = UA_Client_readBrowseNameAttribute(client, nodeId, &bn);
    if (retval == UA_STATUSCODE_GOOD) {
        memset(tempStr, 0, sizeof(tempStr)); memcpy(tempStr, bn.name.data, bn.name.length);
        cJSON* browsename = cJSON_CreateObject();
        cJSON_AddNumberToObject(browsename, "namespaceindex", bn.namespaceIndex);
        cJSON_AddStringToObject(browsename, "name", tempStr);
        cJSON_AddItemToObject(root, "browsename", browsename);
    }
    else {
        cJSON_AddStringToObject(root, "browsename", "Read failed!");
    }
    retval =UA_Client_readDisplayNameAttribute(client, nodeId, &dn);
    if (retval == UA_STATUSCODE_GOOD) {
        cJSON* displayname = cJSON_CreateObject();
        memset(tempStr, 0, sizeof(tempStr)); memcpy(tempStr, dn.text.data, dn.text.length);
        cJSON_AddStringToObject(displayname, "text", tempStr);
        memset(tempStr, 0, sizeof(tempStr)); memcpy(tempStr, dn.locale.data, dn.locale.length);
        cJSON_AddStringToObject(displayname, "locale", tempStr);
        cJSON_AddItemToObject(root, "displayname", displayname);
    }
    else {
        cJSON_AddStringToObject(root, "displayname", "Read failed!");
    }

    retval = UA_Client_readDescriptionAttribute(client, nodeId, &desc);
    if (retval == UA_STATUSCODE_GOOD) {
        cJSON* description = cJSON_CreateObject();
        memset(tempStr, 0, sizeof(tempStr)); memcpy(tempStr, desc.locale.data, desc.locale.length);
        cJSON_AddStringToObject(description, "text", tempStr);
        memset(tempStr, 0, sizeof(tempStr)); memcpy(tempStr, desc.text.data, desc.text.length);
        cJSON_AddStringToObject(description, "locale", tempStr);
        cJSON_AddItemToObject(root, "description", description);
    }
    else {
        cJSON_AddStringToObject(root, "description", "Read failed!");
    }
    retval = UA_Client_readWriteMaskAttribute(client, nodeId, &wm);
    if (retval == UA_STATUSCODE_GOOD) {
        cJSON_AddNumberToObject(root, "writemask", wm);
    }
    else {
        cJSON_AddStringToObject(root, "writemask", "Read failed!");
    }
    retval = UA_Client_readUserWriteMaskAttribute(client, nodeId, &uwm);
    if (retval == UA_STATUSCODE_GOOD) {
        cJSON_AddNumberToObject(root, "userwritemask", uwm);
    }
    else {
        cJSON_AddStringToObject(root, "userwritemask", "Read failed!");
    }
    cJSON* jsonNode = nodeIdToJson(nodeId);
    cJSON_AddItemToObject(root, "nodeid", jsonNode);  
}

void retriveVariableAttribute(UA_Client* client, UA_NodeId nodeId, cJSON* root) {
    UA_Int32 vr;
    size_t arrayDimSize = 0;
    UA_UInt32* arrayDim = 0;
    UA_Byte al;
    UA_Byte ual;
    UA_UInt32 alx;
    UA_Double msi;
    UA_Boolean his;
    UA_NodeId dt;
    retriveBasicAttribute(client,nodeId,root);
    UA_Client_readValueRankAttribute(client, nodeId, &vr);
    UA_Client_readArrayDimensionsAttribute(client, nodeId, &arrayDimSize, &arrayDim);
    UA_Client_readAccessLevelAttribute(client, nodeId, &al);
    UA_Client_readUserAccessLevelAttribute(client, nodeId, &ual);
    UA_Client_readAccessLevelExAttribute(client, nodeId, &alx);
    UA_Client_readMinimumSamplingIntervalAttribute(client, nodeId, &msi);
    UA_Client_readHistorizingAttribute(client, nodeId, &his);
    UA_Client_readDataTypeAttribute(client, nodeId, &dt);

    cJSON_AddNumberToObject(root, "valuerank", vr);
    cJSON* jsonArray = cJSON_CreateArray();
    for (int i = 0; i < arrayDimSize; i++) {
        cJSON_AddItemToArray(jsonArray, cJSON_CreateNumber(arrayDim[i]));
    }
    cJSON_AddItemToObject(root, "arraydimensions", jsonArray);
    cJSON_AddNumberToObject(root, "accesslevel", al);
    cJSON_AddNumberToObject(root, "useraccesslevel", ual);
    cJSON_AddNumberToObject(root, "accesslevelex", alx);
    cJSON_AddNumberToObject(root, "minimumsamplinginterval", msi);
    cJSON_AddNumberToObject(root, "historizing", his);
    cJSON_AddItemToObject(root,"datatype",nodeIdToJson(dt));

    ///////////////////////////////////////////////////
    UA_ClientConfig* cc = UA_Client_getConfig(client);
    const UA_DataType* type = UA_findDataTypeWithCustom(&dt, cc->customDataTypes);
    if (type == 0) {
        buildCustomDataType(client, dt);
    }
    UA_Variant value; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&value);
    UA_StatusCode retval = UA_Client_readValueAttribute(client, nodeId, &value);
    cJSON* jsonValueNode = cJSON_CreateObject();
    if (retval == UA_STATUSCODE_GOOD && value.data != 0) {
        //const UA_DataType* type = UA_findDataTypeWithCustom(&dt, cc->customDataTypes);
        if (value.type->typeKind == UA_DATATYPEKIND_EXTENSIONOBJECT) {
            /* if don't have customDataTypes , the raw data will stored at eo->content.encoded.body.data*/
            UA_ExtensionObject* eo = (UA_ExtensionObject*)value.data;
            cJSON_AddStringToObject(jsonValueNode, "FAIL", "unrecognize datatype!");
            printf("TODO: extract value.data\n");
        }
        else {
            extractDataType(&value, jsonValueNode);
        }
    }
    cJSON_AddItemToObject(root, "value", jsonValueNode);
    UA_Variant_clear(&value);
    UA_Variant_init(&value);
    ////////////////////////////////////////////////////

}

void retriveObjectAttribute(UA_Client* client, UA_NodeId nodeId, cJSON* root) {
    UA_Byte en;
    retriveBasicAttribute(client, nodeId, root);
    UA_Client_readEventNotifierAttribute(client, nodeId, &en);
    cJSON_AddNumberToObject(root, "eventnotifier", en);

}

void retriveReferenceAttribute(UA_Client* client, UA_NodeId nodeId, cJSON* root) {
    UA_Boolean sym;
    UA_Boolean isabstract;
    retriveBasicAttribute(client, nodeId, root);
    UA_Client_readSymmetricAttribute(client, nodeId, &sym);
    UA_Client_readIsAbstractAttribute(client, nodeId, &isabstract);
    cJSON_AddNumberToObject(root, "symmetric", sym);
    cJSON_AddNumberToObject(root, "isabstract", isabstract);

}

void retriveDataTypeAttribute(UA_Client* client, UA_NodeId nodeId, cJSON* root) {
    UA_Boolean isabstract;
    retriveBasicAttribute(client, nodeId, root);
    UA_Client_readIsAbstractAttribute(client, nodeId, &isabstract);
    cJSON_AddNumberToObject(root, "isabstract", isabstract);
}

void retriveMethodAttribute(UA_Client* client, UA_NodeId nodeId, cJSON* root) {
    UA_Boolean exe;
    UA_Boolean usrexe;
    retriveBasicAttribute(client, nodeId, root);
    UA_Client_readExecutableAttribute(client, nodeId, &exe);
    UA_Client_readUserExecutableAttribute(client, nodeId, &usrexe);
    cJSON_AddNumberToObject(root, "executable", exe);
    cJSON_AddNumberToObject(root, "userexecutable", usrexe);

}

void retriveObjectTypeAttribute(UA_Client* client, UA_NodeId nodeId, cJSON* root) {
    UA_Boolean isabstract;
    retriveBasicAttribute(client, nodeId, root);
    UA_Client_readExecutableAttribute(client, nodeId, &isabstract);
    cJSON_AddNumberToObject(root, "isabstract", isabstract);
}

void retriveAttribute(UA_Client* client, UA_NodeId nodeId, cJSON* root) {
    UA_NodeClass nc;
    UA_Client_readNodeClassAttribute(client, nodeId, &nc);

    cJSON_AddNumberToObject(root, "nodeclass", nc);

    if (nc == UA_NODECLASS_OBJECT) {
        retriveObjectAttribute(client, nodeId, root);
        return;
    }
    if (nc == UA_NODECLASS_VARIABLE) {
        retriveVariableAttribute(client, nodeId, root);
        return;
    }
    if (nc == UA_NODECLASS_REFERENCETYPE) {
        retriveReferenceAttribute(client, nodeId, root);
        return;
    }
    if (nc == UA_NODECLASS_DATATYPE) {
        retriveDataTypeAttribute(client, nodeId, root);
        return;
    }

    if (nc == UA_NODECLASS_METHOD) {
        retriveMethodAttribute(client, nodeId, root);
        return;
    }
    if (nc == UA_NODECLASS_OBJECTTYPE) {
        retriveObjectTypeAttribute(client, nodeId, root);
        return;
    }
    if (nc == UA_NODECLASS_VARIABLETYPE) {
        retriveVariableAttribute(client, nodeId, root);
        return;
    }

    printf("retriveAttribute not imeplement @  NodeClass %d\n", nc);
}

void browseRefernce(UA_Client* client, UA_NodeId nodeId, cJSON* root) {
    char tempStr[256];
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;

    UA_NodeId_copy(&nodeId, &bReq.nodesToBrowse[0].nodeId);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    cJSON* jsonRefArray = cJSON_CreateArray();
    for (size_t i = 0; i < bResp.resultsSize; ++i) {
        for (size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
            UA_ReferenceDescription* ref = &(bResp.results[i].references[j]);
            cJSON* jsonRef = cJSON_CreateObject();
            cJSON* jsonNode = nodeIdToJson(ref->nodeId.nodeId);
            cJSON_AddItemToObject(jsonRef, "nodeid", jsonNode);
            cJSON* jsonRefType = nodeIdToJson(ref->referenceTypeId);
            cJSON_AddItemToObject(jsonRef, "reftype", jsonRefType);

            cJSON* jsonDisp = cJSON_CreateObject();
            memset(tempStr, 0, sizeof(tempStr)); memcpy(tempStr, ref->displayName.text.data, ref->displayName.text.length);
            cJSON_AddStringToObject(jsonDisp, "text", tempStr);
            memset(tempStr, 0, sizeof(tempStr)); memcpy(tempStr, ref->displayName.locale.data, ref->displayName.locale.length);
            cJSON_AddStringToObject(jsonDisp, "locale", tempStr);
            cJSON_AddItemToObject(jsonRef, "displayname", jsonDisp);

            cJSON_AddItemToArray(jsonRefArray, jsonRef);
            /*
            memset(displayname, 0, sizeof(displayname));
            memset(browsename, 0, sizeof(browsename));
            memcpy(displayname, ref->displayName.text.data, (int)ref->displayName.text.length);
            memcpy(browsename, ref->browseName.name.data, (int)ref->browseName.name.length);
            for (int i = 0; i < depth; i++) {
                printf("\t");
            }
            UA_NodeId hasTypeid = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
            UA_NodeId orgnizeid = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
            UA_NodeId propertyid = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
            UA_NodeId componentid = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
            UA_NodeId modelingruleid = UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE);

            if (UA_NodeId_equal(&ref->referenceTypeId, &hasTypeid)) {
                printf("(T)%s %s\n", displayname, browsename);
            }
            else if (UA_NodeId_equal(&ref->referenceTypeId, &orgnizeid)) {
                printf("-->%s %s \n", displayname, browsename);
            }
            else if (UA_NodeId_equal(&ref->referenceTypeId, &propertyid)) {
                printf("(P)%s %s \n", displayname, browsename);
            }
            else if (UA_NodeId_equal(&ref->referenceTypeId, &componentid)) {
                printf("(C)%s %s \n", displayname, browsename);
            }
            else if (UA_NodeId_equal(&ref->referenceTypeId, &modelingruleid)) {
                printf("(MR)%s %s \n", displayname, browsename);
            }
            else {
                printf("%s %s \n", displayname, browsename);
            }
            if (ref->nodeClass == UA_NODECLASS_VARIABLE)
            {
                UA_Variant* var = UA_Variant_new();
                UA_Client_readValueAttribute(client, ref->nodeId.nodeId, var);
                //Device* p = (Device*)var->data;
                UA_Variant_delete(var);
                //UA_ReadResponse_clear(resp);
            }
            else if (ref->nodeClass == UA_NODECLASS_OBJECT) {
                traverse(client, ref->nodeId.nodeId, depth + 1);
            }
            */
        }
    }
    cJSON_AddItemToObject(root, "references", jsonRefArray);
    retriveAttribute(client, nodeId, root);
    char* str = cJSON_Print(root);
    printf("%s\n", str);
    free(str);
    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);
}

void browseRoot(UA_Client* client, cJSON* root) {
    browseRefernce(client, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER), root);
};

UA_NodeId jsonToNodeId(cJSON* nodeidObj) {
    UA_NodeId nodeId;
    cJSON* nameidx = cJSON_GetObjectItem(nodeidObj, "namespaceindex");
    cJSON* idtype = cJSON_GetObjectItem(nodeidObj, "identifiertype");
    if (idtype->valuedouble == UA_NODEIDTYPE_NUMERIC ||
        idtype->valuedouble == UA_NODEIDTYPE_NUMERIC + 1 ||
        idtype->valuedouble == UA_NODEIDTYPE_NUMERIC + 2) {
        cJSON* id = cJSON_GetObjectItem(nodeidObj, "identifier");
        nodeId = UA_NODEID_NUMERIC((UA_UInt16)nameidx->valuedouble, (UA_UInt32)(id->valuedouble));
    }
    if (idtype->valuedouble == UA_NODEIDTYPE_STRING) {
        cJSON* id = cJSON_GetObjectItem(nodeidObj, "identifier");
        nodeId = UA_NODEID_STRING((UA_UInt16)nameidx->valuedouble, id->valuestring);
    }
    if (idtype->valuedouble == UA_NODEIDTYPE_GUID) {
        cJSON* id = cJSON_GetObjectItem(nodeidObj, "identifier");
        UA_Guid guid;
        UA_String str = UA_STRING(id->valuestring);
        UA_Guid_parse(&guid, str);
        nodeId = UA_NODEID_GUID((UA_UInt16)nameidx->valuedouble, guid);
    }
    if (idtype->valuedouble == UA_NODEIDTYPE_BYTESTRING) {
        cJSON* id = cJSON_GetObjectItem(nodeidObj, "identifier");
        nodeId = UA_NODEID_BYTESTRING((UA_UInt16)nameidx->valuedouble, id->valuestring);
    }
    return nodeId;
}

int handleBrowse(UA_Client* client, cJSON* obj, cJSON* result_obj) {
    cJSON* nodeidObj = cJSON_GetObjectItem(obj, "nodeid");
    UA_NodeId nodeId = jsonToNodeId(nodeidObj);
    browseRefernce(client, nodeId, result_obj);
    return 0;
}

int handleAddMonitorAttribute(UA_Client* client,cJSON* obj,void * socket_context) {
    cJSON* nodeidObj = cJSON_GetObjectItem(obj, "nodeid");
    UA_NodeId nodeId = jsonToNodeId(nodeidObj);
    addMonitoredItemToVariable(client, nodeId, socket_context);
    return 0;
}

int handleDelMonitorItem(UA_Client* client, cJSON* obj) {
    cJSON* subObj = cJSON_GetObjectItem(obj, "subid");
    cJSON* monObj = cJSON_GetObjectItem(obj, "monid");
    deleteMonitoredItems(client, (UA_UInt32)subObj->valueint, (UA_UInt32)monObj->valueint);
    for (int i = 0; i < sizeof(mctx) / sizeof(struct moniterdContext); i++) {
        if (mctx[i].moniterid == (UA_UInt32)monObj->valueint && mctx[i].subid == (UA_UInt32)subObj->valueint) {
            if (mctx[i].id.identifierType == UA_NODEIDTYPE_STRING || mctx[i].id.identifierType == UA_NODEIDTYPE_BYTESTRING) {
                free(mctx[i].id.identifier.string.data);
            }
            memset(&mctx[i], 0, sizeof(struct moniterdContext));
        }
    }
    return 0;
}

struct moniterdContext *  findEmptyCtx() {
    for (int i = 0; i < sizeof(mctx) / sizeof(struct moniterdContext); i++) {
        if (!mctx[i].occupied){
            return &mctx[i];
        }
    }
    return 0;
}


static void handler_DataChanged(UA_Client* client, UA_UInt32 subId,
    void* subContext, UA_UInt32 monId,
    void* monContext, UA_DataValue* value)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Received Notification");
    //////////Get monitored node id first
    struct moniterdContext* ctx = (struct moniterdContext*)monContext;

    /////////


    void* sock_ptr = ctx->sock_ptr;
    cJSON* jsonValueNode = cJSON_CreateObject();
    UA_ClientConfig* cc = UA_Client_getConfig(client);
    if(value->value.type->typeKind == UA_DATATYPEKIND_EXTENSIONOBJECT){
        /* if don't have customDataTypes , the raw data will stored at eo->content.encoded.body.data*/
        UA_ExtensionObject* eo = (UA_ExtensionObject*)value->value.data;
        cJSON_AddStringToObject(jsonValueNode, "FAIL", "unrecognize datatype!");
        printf("TODO: extract value.data\n");
    }
    else {
        extractDataType(&value->value, jsonValueNode);
    }
    cJSON_AddStringToObject(jsonValueNode, "answertype", "ASYNC");
    cJSON* jsonNode = nodeIdToJson(ctx->id);
    cJSON_AddItemToObject(jsonValueNode, "nodeid", jsonNode);
    cJSON_AddNumberToObject(jsonValueNode, "subid", subId);
    cJSON_AddNumberToObject(jsonValueNode, "monid", monId);
    sendJsonObj(jsonValueNode, sock_ptr);
    
    char* str = cJSON_Print(jsonValueNode);
    UA_DateTimeStruct dts = UA_DateTime_toStruct(value->sourceTimestamp);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SubId:%u, MonId:%u, Current Value: %s, date is: %u-%u-%u %u:%u:%u.%03u\n",
            subId, monId, str,
            dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    free(str);
    cJSON_Delete(jsonValueNode);
}



void addMonitoredItemToVariable(UA_Client* client, UA_NodeId target_nodeid,void* sock_ptr)
{
    UA_NodeId dt;
    UA_ClientConfig* cc = UA_Client_getConfig(client);
    UA_Client_readDataTypeAttribute(client, target_nodeid, &dt);
    const UA_DataType* type = UA_findDataTypeWithCustom(&dt, cc->customDataTypes);
    if (type == 0) {
        buildCustomDataType(client, dt);
    }


    /* Create a subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedPublishingInterval = 100; 


    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
        NULL, NULL, NULL);

    UA_UInt32 subId = response.subscriptionId;
    if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Create subscription succeeded, id %u\n", subId);
    }


    UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(target_nodeid);

    monRequest.requestedParameters.samplingInterval = 0;
    monRequest.requestedParameters.queueSize = 0;
   // monRequest.itemToMonitor.attributeId = UA_ATTRIBUTEID_DISPLAYNAME;

    struct moniterdContext* ctx = findEmptyCtx();

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
            UA_TIMESTAMPSTORETURN_BOTH,
            monRequest, ctx,
            handler_DataChanged, NULL);
    if (monResponse.statusCode == UA_STATUSCODE_GOOD)
    {
        /*TODO: clean ctx when delete monitor item*/
        ctx->subid = subId;
        ctx->moniterid = monResponse.monitoredItemId;
        ctx->sock_ptr = sock_ptr;
        ctx->id = target_nodeid;
        if (ctx->id.identifierType == UA_NODEIDTYPE_STRING || ctx->id.identifierType == UA_NODEIDTYPE_BYTESTRING) {
            char * temp = (char*)malloc(ctx->id.identifier.string.length);
            memcpy(temp, ctx->id.identifier.string.data, ctx->id.identifier.string.length);/*TODO: prevent memory leak!!*/
            ctx->id.identifier.string.data = temp;
        }

        ctx->occupied = true;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Monitoring 'the.answer', id %u\n",
            monResponse.monitoredItemId);

    }

}


void deleteMonitoredItems(UA_Client * client,UA_UInt32 subid, UA_UInt32 monid)
{
    UA_Client_MonitoredItems_deleteSingle(client, subid, monid);
    UA_Client_Subscriptions_deleteSingle(client, subid);
}
