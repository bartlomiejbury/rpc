#include <stdio.h>
#include <event2/event.h>
#include "rpc_client.h"

void on_time(const char *topic,const char *payload,void *user_data) {
    printf("[%s] %s\n",topic,payload);
}

int main() {
    struct event_base *base = event_base_new();

    RPCPubSubClient *client = rpc_client_new("test");

    rpc_subscribe(client,"time",on_time,NULL);
    struct event *sub_event = rpc_client_add_to_loop(client, base);

    char resp1[128], resp2[128];
    char err1[256], err2[256];
    int code1 = 0, code2 = 0;

    int rc1 = rpc_call(client,3000,resp1,sizeof(resp1), err1, sizeof(err1), &code1, "add", "ii", 5,3);
    if(rc1 == RPC_RESULT_OK && code1 == 0) {
        printf("add(5,3) = %s\n",resp1);
    } else {
        printf("add(5,3) failed: errorCode=%d, errorMsg=%s\n", code1, err1);
    }

    int rc2 = rpc_call(client,3000,resp2,sizeof(resp2), err2, sizeof(err2), &code2, "multiply", "ii", 4,2);
    if(rc2 == RPC_RESULT_OK && code2 == 0) {
        printf("multiply(4,2) = %s\n",resp2);
    } else {
        printf("multiply(4,2) failed: errorCode=%d, errorMsg=%s\n", code2, err2);
    }

    event_base_dispatch(base);
    rpc_client_free(client);
    event_free(sub_event);
    event_base_free(base);
}