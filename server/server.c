#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "../open62541.h"

static UA_INLINE UA_ByteString
loadFile(const char *const path) {
    UA_ByteString fileContents = UA_STRING_NULL;

    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        errno = 0; /* We read errno also from the tcp layer... */
        return fileContents;
    }

    /* Get the file length, allocate the data and read */
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte *)UA_malloc(fileContents.length * sizeof(UA_Byte));
    if(fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if(read != fileContents.length)
            UA_ByteString_clear(&fileContents);
    } else {
        fileContents.length = 0;
    }
    fclose(fp);

    return fileContents;
}


int main(void) {
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    /* Load certificate and private key */
    certificate = loadFile("cert/server_cert.der");
    privateKey = loadFile("cert/server_key.der");


    //signal(SIGINT, stopHandler);
    //signal(SIGTERM, stopHandler);
    UA_Float sig[28];
    UA_Byte evt[13];
    UA_Server* server = UA_Server_new();
    UA_ServerConfig* config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    //UA_ServerConfig_setMinimal(config, 14840, NULL);
    size_t issuerListSize = 0;
    UA_ByteString *issuerList = NULL;
    /* Revocation lists are supported, but not used for the example here */
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;
    size_t trustListSize = 1;
    //UA_ByteString *trustList = NULL;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    for(size_t trustListCount = 0; trustListCount < trustListSize; trustListCount++)
        trustList[trustListCount] = loadFile("cert/client_cert.der");


    UA_StatusCode retval = UA_ServerConfig_setDefaultWithSecurityPolicies(config, 14840,
                                                       &certificate, &privateKey,
                                                       trustList, trustListSize,
                                                       issuerList, issuerListSize,
                                                       revocationList, revocationListSize);

    /* important! accept all or put certificate in trust list */

    if(config->secureChannelPKI.clear)
        config->secureChannelPKI.clear(&config->secureChannelPKI);
    UA_CertificateVerification_AcceptAll(&config->secureChannelPKI);

    if(config->sessionPKI.clear)
        config->sessionPKI.clear(&config->sessionPKI);
    UA_CertificateVerification_AcceptAll(&config->sessionPKI);



    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);


    UA_Server_run_startup(server);
    int loop = 0;
    while (1)
    {
        UA_Server_run_iterate(server, false);
        usleep(10);//usleep(10);
    }

    UA_Server_delete(server);
    return EXIT_SUCCESS;
}
