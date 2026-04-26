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
    rpc_call(client,3000,resp1,sizeof(resp1), NULL, 0, "add", "ii", 5,3);
    printf("add(5,3) = %s\n",resp1);

    rpc_call(client,3000,resp2,sizeof(resp2), NULL, 0, "multiply", "ii", 4,2);
    printf("multiply(4,2) = %s\n",resp2);

    event_base_dispatch(base);
    rpc_client_free(client);
    event_free(sub_event);
    event_base_free(base);
}