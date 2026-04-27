#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include <stddef.h>
#include <yyjson.h>

typedef struct RPCServer RPCServer;

// Function pointer type for RPC handlers
// doc: document for creating return values
// params: JSON array value (can contain any types)
// user_data: custom data pointer passed during registration
// result: pointer to store the result value (set *result on success)
// err_msg: buffer to write optional error message
// err_msg_size: size of err_msg buffer
// Returns: error code (0 for success, non-zero for error)
typedef int (*rpc_handler_t)(
    yyjson_mut_doc *doc,
    yyjson_val *params,
    void *user_data,
    yyjson_mut_val **result,
    char *err_msg,
    size_t err_msg_size
);

// Create a new RPC server
RPCServer* rpc_server_new(const char *name);

// Register an RPC function with optional user data
void rpc_server_register(RPCServer *s, const char *name, rpc_handler_t handler, void *user_data);

// Start the server (spawns background thread for RPC loop)
int rpc_server_start(RPCServer *s);

// Publish a message to a topic
void rpc_server_publish(RPCServer *s, const char *topic, const char *json_msg);

// Stop the server and clean up
void rpc_server_stop(RPCServer *s);

// Free the server
void rpc_server_free(RPCServer *s);

#endif // RPC_SERVER_H
