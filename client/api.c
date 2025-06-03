#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "general.h"
#include <ctype.h>
#include <sys/ioctl.h>


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

int checkHeader(int connfd) {
    int loop = 0;
    char org[1024*32];
    int n = 0;

    memset(org, 0, sizeof(org));
    while (1) {
        loop++;
        n = recv(connfd, org, sizeof(org), MSG_PEEK);
        if(n == 0){// n == 0 means remote gracefully closed socket
            printf("n:%d errno:%d loop:%d\n", n, errno, loop);
            return -1;
        }
        if (n < 0) {
            if (errno == EAGAIN) {
                if (loop < 10) {
                    //printf("n=%d errno=%d connfd = %lld \n", n, err, connfd);
                    usleep(100000);
                    continue;
                } else {
                    return 0;
                }
            }
            else { // other error
                printf("n:%d errno:%d loop:%d\n", n, errno, loop);
                close(connfd);
                return -1;
            }
        }
        printf("PEEK:[%s] n=%d\n", org, n);
        return parseJson(org, &n);
    }
    return 0;
}

int sendJsonObj(cJSON* result_obj , void *sock_ptr) {
    int s = *((int*)sock_ptr);
    char* outstr = cJSON_Print(result_obj);
    send(s, outstr, (int)(strlen(outstr) + 1), 0);
    free(outstr);
    return 0;
}


void apiServer(UA_Client* client)
{
    int sock_fd, new_fd;
    socklen_t addrlen;
    struct sockaddr_in my_addr, client_addr;
    int status;
    char indata[1024] = {0};
    int on = 1;

    // create a socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Socket creation error");
        exit(1);
    }

    // for "Address already in use" error message
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1) {
        perror("Setsockopt error");
        exit(1);
    }

    // non-block
    if( ioctl(sock_fd, FIONBIO, (char *)&on) <0)
    {
        printf("ioctl() faild %d\n",sock_fd);
        close(sock_fd);
        exit(1);
    }


    // server address
    my_addr.sin_family = AF_INET;
    inet_aton(host, &my_addr.sin_addr);
    my_addr.sin_port = htons(port);

    status = bind(sock_fd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if (status == -1) {
        perror("Binding error");
        exit(1);
    }
    printf("server start at: %s:%d\n", inet_ntoa(my_addr.sin_addr), port);

    status = listen(sock_fd, 5);
    if (status == -1) {
        perror("Listening error");
        exit(1);
    }
    printf("wait fqor connection...\n");

    addrlen = sizeof(client_addr);
    while (1) {
        new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addrlen);
        if(new_fd < 0){
            if (errno == EAGAIN) {
                UA_Client_run_iterate(client, 50);
                continue;
            } else {
                perror("accept error");
                exit(1);
            }
        }
        if( ioctl(new_fd, FIONBIO, (char *)&on) <0)
        {
            printf("ioctl() faild %d\n",new_fd);
            close(new_fd);
            exit(1);
        }

        printf("connected by %s:%d\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

        while (1) {
            int hlen = checkHeader(new_fd);
            if(hlen == 0) {
                printf("not ready header\n");
                UA_Client_run_iterate(client, 50);
                continue;
            }
            if (hlen < 0) {
                printf("socket closed\n");
                break;
            }

            int nbytes = recv(new_fd, indata, hlen, 0);
            if (nbytes != hlen) {
                close(new_fd);
                printf("client closed connection.\n");
                break;
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
                send(new_fd, outstr, (int)(strlen(outstr) + 1), 0);
                free(outstr);
                cJSON_Delete(result_obj);
            }

            if (!strcmp(cmd->valuestring, "addmonitorattribute")) {
                handleAddMonitorAttribute(client, obj, (void*) & new_fd);
            }
            if (!strcmp(cmd->valuestring, "delmonitoritem")) {
                handleDelMonitorItem(client, obj);
             }

            cJSON_Delete(obj);
        }
    }
    close(sock_fd);

    return;
}

