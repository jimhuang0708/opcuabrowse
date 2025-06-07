#ifndef __GENERAL_H__
#define __GENERAL_H__
#include "open62541.h"
#include "cJSON.h"

struct moniterdContext {
	int occupied;
	int subid;
	int moniterid;
	UA_NodeId id;
	void* sock_ptr;
};

UA_ByteString loadFile(const char* const path);
UA_StatusCode UA_Client_readDataTypeDefinitionAttribute(UA_Client* client, const UA_NodeId nodeId, UA_StructureDefinition* outDataTypeDefinition);
void retriveVariableAttribute(UA_Client* client, UA_NodeId nodeId, cJSON* root);
void retriveAttribute(UA_Client* client, UA_NodeId nodeId, cJSON* root);
void retriveObjectAttribute(UA_Client* client, UA_NodeId nodeId, cJSON* root);
void UA_Guid_to_hex(const UA_Guid* guid, unsigned char* out, UA_Boolean lower);
void buildCustomDataType(UA_Client* client, UA_NodeId nodeId);
void extractCustomDataType(UA_Client* client, void* p);
void browseRoot(UA_Client* client, cJSON* root);
void apiServer(UA_Client* client);
int handleBrowse(UA_Client* client,cJSON* obj, cJSON* result_obj);
int handleAddMonitorAttribute(UA_Client* client, cJSON* obj, void* socket_context);
void extractComplexDataTypeArray(void* pData, const UA_DataType* type, size_t arrayLength, char* key, cJSON* jsonParentNode);
void addMonitoredItemToVariable(UA_Client* client, UA_NodeId target_nodeid, void* sock_ptr);
int sendJsonObj(cJSON* result_obj, void* sock_ptr);
int handleDelMonitorItem(UA_Client* client, cJSON* obj);
int handleWriteValue(UA_Client* client, cJSON* obj);
void deleteMonitoredItems(UA_Client* client, UA_UInt32 subid, UA_UInt32 monid);
void addMonitoredItemToEvent(UA_Client* client, UA_NodeId target_nodeid, void* sock_ptr);
void encodeDataType(cJSON* jSrc, UA_Variant* value, const UA_DataType* type);
void encodeComplexDataTypeArray(void* pData, const UA_DataType* type, size_t arrayLength, cJSON* jArray);
cJSON* nodeIdToJson(UA_NodeId nodeId);
UA_NodeId jsonToNodeIdAlloc(cJSON* nodeidObj);

extern char* bytes_to_hex(const unsigned char* bytes, size_t len);
extern unsigned char* hex_to_bytes(const char* hex_str, size_t* len);
extern void epoch_to_iso8601(long long milliseconds, char* buffer, size_t buffer_size);
long long iso8601_to_epoch(const char* iso_string);
void writeVariable(UA_Client* client, UA_NodeId target_nodeid, const UA_Variant* p);
#endif
