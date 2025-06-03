#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "general.h"
// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

const char* host = "0.0.0.0";
int port = 7000;


int
parseJson(char* org, int* orgBuflen) {
    int i = 0;
    int token_scope = 0;
    int token_array = 0;
    if (*orgBuflen == 0) return 0;

    for (i = 0; i < *orgBuflen; i++) {
        if ((!isspace(org[i])) && (org[i] != 0)) {
            break;
        }
    }
    if (i >= *orgBuflen) {
        printf("String empty\n");
        return 0;
    }


    if (org[i] != '{' && org[i] != '[') {
        printf("bad string\n");
        return -1;//It is not json,bad string
    }

    for (; i < *orgBuflen; i++) {
        if (org[i] == '{') token_scope++;
        if (org[i] == '}') token_scope--;
        if (org[i] == '[') token_array++;
        if (org[i] == ']') token_array--;
        if (token_scope == 0 && token_array == 0)
            break;
    }
    if (token_scope == 0 && token_array == 0) {
        return i + 1;
    }
    else {
        printf("String not enough len=%d %s\n", *orgBuflen, org);
        return 0;//string not ready
    }
}

int checkHeader(SOCKET connfd) {
    int loop = 0;
    char org[1024*32];
    int n = 0;
    u_long mode = 1;  // 1 to enable non-blocking socket
    ioctlsocket(connfd, FIONBIO, &mode);

    memset(org, 0, sizeof(org));
    while (1) {
        loop++;
        n = recv(connfd, org, sizeof(org), MSG_PEEK);
        if (n <= 0) {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) {
                if (n < 0 && loop < 10) {
                    //printf("n=%d errno=%d connfd = %lld \n", n, err, connfd);
                    Sleep(100);
                    continue;
                }
                else {
                    return 0;
                }
            }
            else { // n == 0 means remote gracefully closed socket
                printf("n:%d errno:%d loop:%d\n", n, err, loop);
                closesocket(connfd);
                return -1;
            }
        }
        printf("PEEK:[%s] n=%d\n", org, n);
        return parseJson(org, &n);
    }
    return 0;
}

int sendJsonObj(cJSON* result_obj , void *sock_ptr) {
    SOCKET s = *((SOCKET*)sock_ptr);
    char* outstr = cJSON_Print(result_obj);
    send(s, outstr, (int)(strlen(outstr) + 1), 0);
    free(outstr);
    return 0;
}

void apiServer(UA_Client* client)
{
    SOCKET sock, new_sock;
    socklen_t addrlen;
    struct sockaddr_in my_addr, client_addr;
    int status;
    char indata[1024] = { 0 };
    int on = 1;

    // init winsock
    WSADATA wsa = { 0 };
    WORD wVer = MAKEWORD(2, 2);
    WSAStartup(wVer, &wsa);
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != NO_ERROR) {
        printf("Error: init winsock\n");
        exit(1);
    }

    // create a socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        perror("Socket creation error");
        exit(1);
    }

    // for "Address already in use" error message
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(int)) == -1) {
        perror("Setsockopt error");
        exit(1);
    }

    u_long mode = 1;  // 1 to enable non-blocking socket
    ioctlsocket(sock, FIONBIO, &mode);

    // server address
    my_addr.sin_family = AF_INET;
    inet_pton(AF_INET, host, &my_addr.sin_addr);
    my_addr.sin_port = htons(port);

    status = bind(sock, (struct sockaddr*)&my_addr, sizeof(my_addr));
    if (status == -1) {
        perror("Binding error");
        exit(1);
    }
    char my_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &my_addr.sin_addr, my_ip, sizeof(my_ip));
    printf("server start at: %s:%d\n", my_ip, port);

    status = listen(sock, 5);
    if (status == -1) {
        perror("Listening error");
        exit(1);
    }
    printf("wait for connection...\n");

    addrlen = sizeof(client_addr);
    while (1) {
        new_sock = accept(sock, (struct sockaddr*)&client_addr, &addrlen);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        
        int err = WSAGetLastError();
        if ( err == 0) {
            printf("connected by %s:%d\n", client_ip, ntohs(client_addr.sin_port));
            ioctlsocket(new_sock, FIONBIO, &mode);
        } else if (err == WSAEWOULDBLOCK) {
            UA_Client_run_iterate(client, 0);
            continue;
        } else {
            perror("accept error");
            exit(1);
        }

        while (1) {
            int hlen = checkHeader(new_sock);
            if(hlen == 0) {
                printf("not ready header\n");
                UA_Client_run_iterate(client, 0);
                continue;
            }
            if (hlen < 0) {
                printf("socket closed\n");
                break;
            }
            int nbytes = recv(new_sock, indata, hlen, 0);
            if (nbytes != hlen) {
                perror("socket error!");
                exit(1);
            }
            indata[nbytes] = 0;
            printf("recv: %s\n", indata);
            cJSON* obj = cJSON_Parse(indata);
            if (obj == NULL)
            {
                printf("parse fail.\n");
                //sprintf(outdata, "{ \"result\" : \"error\" }");
                //send(new_sock, outdata, strlen(outdata) + 1 , 0);//include 0 at string end
                continue;
            }

            cJSON* cmd = cJSON_GetObjectItem(obj, "cmd");
            printf("cmd: %s\n", cmd->valuestring);
            if (!strcmp(cmd->valuestring, "browse")) {
                cJSON* result_obj = cJSON_CreateObject();
                handleBrowse(client, obj, result_obj);
                cJSON_AddStringToObject(result_obj,"answertype","RESPONSE");
                char* outstr = cJSON_Print(result_obj);
                //include 0 at string end
                send(new_sock, outstr, (int)(strlen(outstr) + 1), 0);
                free(outstr);
                cJSON_Delete(result_obj);
            }

            if (!strcmp(cmd->valuestring, "addmonitorattribute")) {
                handleAddMonitorAttribute(client, obj, (void*) & new_sock);
            }
            if (!strcmp(cmd->valuestring, "delmonitoritem")) {
                handleDelMonitorItem(client, obj);
             }
            cJSON_Delete(obj);
        }
    }
    closesocket(sock);
    WSACleanup();

    return ;
}
