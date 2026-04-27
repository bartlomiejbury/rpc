#include "rpc_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <zmq.h>
#include <yyjson.h>

#define MAX_FUNCS 32
#define MAX_NAME_LEN 64

typedef struct {
    char name[MAX_NAME_LEN];
    rpc_handler_t handler;
    void *user_data;
} RPCFunction;

struct RPCServer {
    char *rpc_addr;
    char *pub_addr;

    void *context;
    void *rep;
    void *pub;

    RPCFunction funcs[MAX_FUNCS];
    int func_count;

    pthread_t rpc_thread;
    int running;
    pthread_mutex_t mutex;
};

static void* rpc_loop(void *arg) {
    RPCServer *s = (RPCServer*)arg;

    while (s->running) {
        zmq_msg_t msg;
        zmq_msg_init(&msg);

        int rc = zmq_msg_recv(&msg, s->rep, ZMQ_DONTWAIT);
        if (rc == -1) {
            usleep(10000); // 10ms
            zmq_msg_close(&msg);
            continue;
        }

        const char *request_str = (const char*)zmq_msg_data(&msg);
        size_t request_len = zmq_msg_size(&msg);

        yyjson_doc *doc = yyjson_read((char*)request_str, request_len, 0);
        if (!doc) {
            zmq_msg_close(&msg);
            zmq_send(s->rep, "{\"result\":null,\"errorCode\":2,\"errorMsg\":\"invalid json\"}", 59, 0);
            continue;
        }

        yyjson_val *root = yyjson_doc_get_root(doc);
        yyjson_val *method_val = yyjson_obj_get(root, "method");

        if (!method_val || !yyjson_is_str(method_val)) {
            yyjson_doc_free(doc);
            zmq_msg_close(&msg);
            zmq_send(s->rep, "{\"result\":null,\"errorCode\":1,\"errorMsg\":\"missing method\"}", 61, 0);
            continue;
        }

        const char *method = yyjson_get_str(method_val);
        yyjson_val *params_val = yyjson_obj_get(root, "params");

        rpc_handler_t handler = NULL;
        void *user_data = NULL;
        pthread_mutex_lock(&s->mutex);
        for (int i = 0; i < s->func_count; i++) {
            if (strcmp(s->funcs[i].name, method) == 0) {
                handler = s->funcs[i].handler;
                user_data = s->funcs[i].user_data;
                break;
            }
        }
        pthread_mutex_unlock(&s->mutex);

        if (!handler) {
            yyjson_doc_free(doc);
            zmq_msg_close(&msg);
            zmq_send(s->rep, "{\"result\":null,\"errorCode\":3,\"errorMsg\":\"unknown method\"}", 62, 0);
            continue;
        }

        // Call the handler (pass JSON params directly)
        yyjson_mut_doc *resp_doc = yyjson_mut_doc_new(NULL);
        yyjson_mut_val *result = NULL;
        char err_msg[256] = {0};
        int error_code = handler(resp_doc, params_val, user_data, &result, err_msg, sizeof(err_msg));

        // Build response
        yyjson_mut_val *resp_root = yyjson_mut_obj(resp_doc);
        yyjson_mut_doc_set_root(resp_doc, resp_root);

        // Add result (create null if error occurred or handler didn't set result)
        if (error_code != 0 || result == NULL) {
            result = yyjson_mut_null(resp_doc);
        }
        yyjson_mut_obj_add_val(resp_doc, resp_root, "result", result);

        // Add error fields
        yyjson_mut_obj_add_int(resp_doc, resp_root, "errorCode", error_code);
        if (err_msg[0] != '\0') {
            yyjson_mut_obj_add_str(resp_doc, resp_root, "errorMsg", err_msg);
        } else {
            yyjson_mut_obj_add_null(resp_doc, resp_root, "errorMsg");
        }

        // Serialize response
        size_t resp_len;
        char *resp_str = yyjson_mut_write(resp_doc, 0, &resp_len);

        zmq_send(s->rep, resp_str, resp_len, 0);

        free(resp_str);
        yyjson_mut_doc_free(resp_doc);
        yyjson_doc_free(doc);
        zmq_msg_close(&msg);
    }

    return NULL;
}

RPCServer* rpc_server_new(const char *name) {
    RPCServer *s = calloc(1, sizeof(RPCServer));
    if (!s) return NULL;

    char rpc_addr[256];
    char pub_addr[256];
    snprintf(rpc_addr, sizeof(rpc_addr), "ipc:///tmp/%s_rpc.sock", name);
    snprintf(pub_addr, sizeof(pub_addr), "ipc:///tmp/%s_pub.sock", name);

    s->rpc_addr = strdup(rpc_addr);
    s->pub_addr = strdup(pub_addr);
    s->func_count = 0;
    s->running = 0;
    pthread_mutex_init(&s->mutex, NULL);

    s->context = zmq_ctx_new();
    if (!s->context) {
        free(s->rpc_addr);
        free(s->pub_addr);
        free(s);
        return NULL;
    }

    return s;
}

void rpc_server_register(RPCServer *s, const char *name, rpc_handler_t handler, void *user_data) {
    if (!s || s->func_count >= MAX_FUNCS) return;

    pthread_mutex_lock(&s->mutex);
    strncpy(s->funcs[s->func_count].name, name, MAX_NAME_LEN - 1);
    s->funcs[s->func_count].handler = handler;
    s->funcs[s->func_count].user_data = user_data;
    s->func_count++;
    pthread_mutex_unlock(&s->mutex);
}

int rpc_server_start(RPCServer *s) {
    if (!s) return -1;

    s->rep = zmq_socket(s->context, ZMQ_REP);
    if (!s->rep) {
        fprintf(stderr, "Failed to create REP socket\n");
        return -1;
    }

    if (zmq_bind(s->rep, s->rpc_addr) != 0) {
        fprintf(stderr, "Failed to bind REP socket to %s: %s\n", s->rpc_addr, zmq_strerror(errno));
        zmq_close(s->rep);
        return -1;
    }

    s->pub = zmq_socket(s->context, ZMQ_PUB);
    if (!s->pub) {
        fprintf(stderr, "Failed to create PUB socket\n");
        zmq_close(s->rep);
        return -1;
    }

    if (zmq_bind(s->pub, s->pub_addr) != 0) {
        fprintf(stderr, "Failed to bind PUB socket to %s: %s\n", s->pub_addr, zmq_strerror(errno));
        zmq_close(s->rep);
        zmq_close(s->pub);
        return -1;
    }

    printf("RPC Server started on %s\n", s->rpc_addr);
    printf("PUB Server started on %s\n", s->pub_addr);

    s->running = 1;
    if (pthread_create(&s->rpc_thread, NULL, rpc_loop, s) != 0) {
        fprintf(stderr, "Failed to create RPC thread\n");
        zmq_close(s->rep);
        zmq_close(s->pub);
        return -1;
    }

    return 0;
}

void rpc_server_publish(RPCServer *s, const char *topic, const char *json_msg) {
    if (!s || !s->pub) return;

    zmq_send(s->pub, topic, strlen(topic), ZMQ_SNDMORE);
    zmq_send(s->pub, json_msg, strlen(json_msg), 0);
}

void rpc_server_stop(RPCServer *s) {
    if (!s) return;

    printf("Stopping server...\n");
    s->running = 0;

    if (s->rpc_thread) {
        pthread_join(s->rpc_thread, NULL);
    }

    if (s->rep) {
        zmq_close(s->rep);
        s->rep = NULL;
    }

    if (s->pub) {
        zmq_close(s->pub);
        s->pub = NULL;
    }

    printf("Server stopped.\n");
}

void rpc_server_free(RPCServer *s) {
    if (!s) return;

    rpc_server_stop(s);

    if (s->context) {
        zmq_ctx_destroy(s->context);
    }

    pthread_mutex_destroy(&s->mutex);
    free(s->rpc_addr);
    free(s->pub_addr);
    free(s);
}
