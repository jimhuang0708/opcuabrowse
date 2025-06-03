
#include <stdlib.h>
#include "open62541.h"
#include "general.h"

struct moniterdContext mctx[100];

int main(int argc, char** argv) {
    
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    /* Load certificate and private key */
    certificate = loadFile("cert/client_cert.der");
    privateKey = loadFile("cert/client_key.der");
    UA_Client* client = UA_Client_new();
    UA_ClientConfig* cc = UA_Client_getConfig(client);
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    size_t revocationListSize = 0;
    UA_ByteString* revocationList = NULL;
#if 0
    size_t trustListSize = 1;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize + 1);
    for (size_t trustListCount = 0; trustListCount < trustListSize; trustListCount++)
        trustList[trustListCount] = loadFile("cert/server_cert.der");
#else
    size_t trustListSize = 0;
    UA_ByteString* trustList = NULL;
#endif
    /* With encryption enabled, the applicationUri needs to match the URI from
    * the certificate and now we user same cert as server so....*/
    //    UA_String_clear(&cc->clientDescription.applicationUri);
    //    cc->clientDescription.applicationUri = UA_STRING_ALLOC("urn:open62541.server.application");
    
    if (argc != 2) {
        printf("opcua server path required\n");
        return 0;
    }



    UA_StatusCode retval = UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
        trustList, trustListSize,
        revocationList, revocationListSize);
    
    if (cc->certificateVerification.clear) {
        cc->certificateVerification.clear(&cc->certificateVerification);
    }
    UA_CertificateVerification_AcceptAll(&cc->certificateVerification);
    //retval = UA_Client_connect(client, "opc.tcp://192.168.51.118:14840");
    //retval = UA_Client_connect(client, "opc.tcp://DESKTOP-ICN8DTE:53530/OPCUA/SimulationServer");
    retval = UA_Client_connect(client, argv[1]);
    
    if (retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        printf("Bad connections\n");
        return (int)retval;
    }
    memset(&mctx,0,sizeof(mctx));
    apiServer(client);
    UA_Client_delete(client);
    return EXIT_SUCCESS;
}
