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
void addMonitoredItemToVariable(UA_Client* client, UA_NodeId target_nodeid, void* socket_context);
int sendJsonObj(cJSON* result_obj, void* sock_ptr);
int handleDelMonitorItem(UA_Client* client, cJSON* obj);
void deleteMonitoredItems(UA_Client* client, UA_UInt32 subid, UA_UInt32 monid);
#endif
