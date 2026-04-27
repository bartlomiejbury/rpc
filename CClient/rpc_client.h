#pragma once

typedef struct RPCPubSubClient RPCPubSubClient;

#define RPC_RESULT_OK 0
#define RPC_RESULT_MAX_SUBS -1
#define RPC_RESULT_FMT -2
#define RPC_RESULT_SEND -3
#define RPC_RESULT_RECV -4
#define RPC_RESULT_JSON_READ -5
#define RPC_RESULT_RPC_ERR -6

// Create a new RPC client
RPCPubSubClient* rpc_client_new(const char *name);

// Free the client
void rpc_client_free(RPCPubSubClient *c);

// Subscribe to a topic with a callback
int rpc_subscribe(RPCPubSubClient *c,
                  const char *topic,
                  void (*cb)(const char*,const char*,void*),
                  void *user_data);

// Add the client's SUB socket to the event loop
struct event* rpc_client_add_to_loop(RPCPubSubClient *c, struct event_base *base);

// Handle subscribe events (internal)
void handle_sub_event(RPCPubSubClient *c);

// Send an RPC request
// Returns RPC_RESULT_OK on success, error code on failure
// If server returns an error, RPC_RESULT_RPC_ERR is returned and err buffer contains errorMsg
// resp_code: optional pointer to store server's errorCode (can be NULL)
int rpc_call(RPCPubSubClient *c,
             int timeout_ms,
             char *resp,
             size_t resp_size,
             char *err,
             size_t err_size,
             int *resp_code,
             const char *method,
             const char *fmt,
             ...);
