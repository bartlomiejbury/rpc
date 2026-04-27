#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include <zmq.h>
#include <yyjson.h>
#include <event2/event.h>

#include "rpc_client.h"

#define MAX_SUBS 32
#define TOPIC_SIZE 20

typedef struct {
    char topic[TOPIC_SIZE];
    void (*callback)(const char *topic, const char *payload, void *user_data);
    void *user_data;
} Subscription;

struct RPCPubSubClient {
    void *context;
    void *req;
    void *sub;

    Subscription subs[MAX_SUBS];
    int sub_count;
};

RPCPubSubClient* rpc_client_new(const char *name)
{
    RPCPubSubClient *c = calloc(1,sizeof(*c));

    c->context = zmq_ctx_new();

    c->req = zmq_socket(c->context,ZMQ_REQ);
    c->sub = zmq_socket(c->context,ZMQ_SUB);

    char req_addr[256];
    char sub_addr[256];
    snprintf(req_addr, sizeof(req_addr), "ipc:///tmp/%s_rpc.sock", name);
    snprintf(sub_addr, sizeof(sub_addr), "ipc:///tmp/%s_pub.sock", name);

    zmq_connect(c->req,req_addr);
    zmq_connect(c->sub,sub_addr);

    return c;
}

int rpc_subscribe(RPCPubSubClient *c,
                  const char *topic,
                  void (*cb)(const char*,const char*,void*),
                  void *user_data)
{
    if(c->sub_count >= MAX_SUBS)
        return RPC_RESULT_MAX_SUBS;

    zmq_setsockopt(c->sub,ZMQ_SUBSCRIBE,topic,strlen(topic));

    Subscription *s = &c->subs[c->sub_count++];

    strncpy(s->topic,topic,TOPIC_SIZE-1);
    s->callback = cb;
    s->user_data = user_data;

    return RPC_RESULT_OK;
}

void handle_sub_event(RPCPubSubClient *c)
{
    uint32_t zmq_events;
    size_t len = sizeof(zmq_events);

    zmq_getsockopt(c->sub,ZMQ_EVENTS,&zmq_events,&len);

    if(!(zmq_events & ZMQ_POLLIN))
        return;

    while(1)
    {
        zmq_msg_t topic_msg;
        zmq_msg_init(&topic_msg);

        if(zmq_msg_recv(&topic_msg,c->sub,ZMQ_DONTWAIT) == -1)
        {
            zmq_msg_close(&topic_msg);

            if(errno == EAGAIN)
                break;

            return;
        }

        zmq_msg_t payload_msg;
        zmq_msg_init(&payload_msg);

        zmq_msg_recv(&payload_msg,c->sub,0);

        char topic[64];
        char payload[512];

        size_t tsize = zmq_msg_size(&topic_msg);
        size_t psize = zmq_msg_size(&payload_msg);

        memcpy(topic,zmq_msg_data(&topic_msg),tsize);
        topic[tsize] = 0;

        memcpy(payload,zmq_msg_data(&payload_msg),psize);
        payload[psize] = 0;

        for(int i=0;i<c->sub_count;i++)
        {
            if(strcmp(topic,c->subs[i].topic)==0)
            {
                c->subs[i].callback(topic,payload,c->subs[i].user_data);
            }
        }

        zmq_msg_close(&topic_msg);
        zmq_msg_close(&payload_msg);
    }
}

static void sub_event_cb(evutil_socket_t fd, short events, void *arg)
{
    RPCPubSubClient *c = arg;
    handle_sub_event(c);
}

struct event* rpc_client_add_to_loop(RPCPubSubClient *c, struct event_base *base)
{
    int fd;
    size_t size = sizeof(fd);

    zmq_getsockopt(c->sub,ZMQ_FD,&fd,&size);

    struct event *sub_event = event_new(
        base,
        fd,
        EV_READ | EV_PERSIST,
        sub_event_cb,
        c
    );

    event_add(sub_event,NULL);
    return sub_event;
}

static int rpc_vsend(RPCPubSubClient *c,
              const char *method,
              const char *fmt,
              va_list ap)
{
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);

    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc,root);
    yyjson_mut_obj_add_str(doc,root,"method",method);
    yyjson_mut_val *params = yyjson_mut_arr(doc);

    const char *p = fmt;

    while(*p)
    {
        if(*p=='i')
            yyjson_mut_arr_add_int(doc,params,va_arg(ap,int));

        else if(*p=='s')
            yyjson_mut_arr_add_str(doc,params,va_arg(ap,char*));

        else if(*p=='d')
            yyjson_mut_arr_add_real(doc,params,va_arg(ap,double));
        else {
            yyjson_mut_doc_free(doc);
            return RPC_RESULT_FMT;
        }
        p++;
    }

    yyjson_mut_obj_add_val(doc,root,"params",params);

    size_t len;
    char *msg = yyjson_mut_write(doc,0,&len);

    yyjson_mut_doc_free(doc);

    int rc = zmq_send(c->req,msg,len,0);

    free(msg);
    return rc == -1 ? RPC_RESULT_SEND : RPC_RESULT_OK;
}

static int rpc_recv(RPCPubSubClient *c,
             int timeout_ms,
             char *resp,
             size_t resp_size,
             char *err,
             size_t err_size,
             int *resp_code)
{
    zmq_setsockopt(c->req,ZMQ_RCVTIMEO,&timeout_ms,sizeof(timeout_ms));

    zmq_msg_t msg;
    zmq_msg_init(&msg);

    if(zmq_msg_recv(&msg,c->req,0) == -1)
    {
        zmq_msg_close(&msg);
        return RPC_RESULT_RECV;
    }

    size_t size = zmq_msg_size(&msg);

    char *data = malloc(size+1);

    memcpy(data,zmq_msg_data(&msg),size);
    data[size] = 0;

    zmq_msg_close(&msg);

    yyjson_doc *doc = yyjson_read(data,size,0);

    free(data);

    if(!doc)
        return RPC_RESULT_JSON_READ;

    yyjson_val *root = yyjson_doc_get_root(doc);

    yyjson_val *result = yyjson_obj_get(root,"result");
    yyjson_val *error_code_val = yyjson_obj_get(root,"errorCode");
    yyjson_val *error_msg_val = yyjson_obj_get(root,"errorMsg");

    int rc = RPC_RESULT_OK;
    int error_code = 0;

    // Get error code
    if(error_code_val && yyjson_is_int(error_code_val)) {
        error_code = yyjson_get_int(error_code_val);
    }

    // Store error code if caller wants it
    if(resp_code != NULL) {
        *resp_code = error_code;
    }

    // Check if there was an error
    if(error_code != 0) {
        // Copy error message if provided
        if(error_msg_val && yyjson_is_str(error_msg_val) && err != NULL && err_size > 0)
        {
            const char *err_msg = yyjson_get_str(error_msg_val);
            strncpy(err,err_msg,err_size-1);
            err[err_size-1] = 0;
        }
        rc = RPC_RESULT_RPC_ERR;
    }

    // Copy result
    if(result)
    {
        char *r = yyjson_val_write(result,0,NULL);

        strncpy(resp,r,resp_size-1);
        resp[resp_size-1] = 0;

        free(r);
    }

    yyjson_doc_free(doc);
    return rc;
}

int rpc_call(RPCPubSubClient *c,
             int timeout_ms,
             char *resp,
             size_t resp_size,
             char *err,
             size_t err_size,
             int *resp_code,
             const char *method,
             const char *fmt,
             ...)
{
    if (err != NULL && err_size > 0) {
        err[0] = 0;
    }

    va_list ap;
    va_start(ap,fmt);
    int rc = rpc_vsend(c,method,fmt,ap);
    va_end(ap);

    if(rc != RPC_RESULT_OK)
        return rc;

    return rpc_recv(c,timeout_ms,resp,resp_size,err,err_size,resp_code);
}

void rpc_client_free(RPCPubSubClient *c)
{
    zmq_close(c->req);
    zmq_close(c->sub);

    zmq_ctx_destroy(c->context);

    free(c);
}