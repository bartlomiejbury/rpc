#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "rpc_server.h"

static volatile int running = 1;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

// RPC handler for "add"
yyjson_mut_val* add_handler(yyjson_mut_doc *doc, yyjson_val *params, char *err_buf, size_t err_size) {
    // Check if params is an array
    if (!params || !yyjson_is_arr(params)) {
        snprintf(err_buf, err_size, "add requires an array of parameters");
        return NULL;
    }

    size_t param_count = yyjson_arr_size(params);
    if (param_count < 2) {
        snprintf(err_buf, err_size, "add requires 2 parameters");
        return NULL;
    }

    // Get parameters - support both int and float
    yyjson_val *p0 = yyjson_arr_get(params, 0);
    yyjson_val *p1 = yyjson_arr_get(params, 1);

    if (!yyjson_is_num(p0) || !yyjson_is_num(p1)) {
        snprintf(err_buf, err_size, "add requires numeric parameters");
        return NULL;
    }

    double a = yyjson_get_num(p0);
    double b = yyjson_get_num(p1);
    double result = a + b;

    // Return as integer if both inputs were integers and result is whole
    if (yyjson_is_int(p0) && yyjson_is_int(p1)) {
        return yyjson_mut_int(doc, (int)result);
    }
    return yyjson_mut_real(doc, result);
}

// RPC handler for "multiply"
yyjson_mut_val* multiply_handler(yyjson_mut_doc *doc, yyjson_val *params, char *err_buf, size_t err_size) {
    if (!params || !yyjson_is_arr(params)) {
        snprintf(err_buf, err_size, "multiply requires an array of parameters");
        return NULL;
    }

    size_t param_count = yyjson_arr_size(params);
    if (param_count < 2) {
        snprintf(err_buf, err_size, "multiply requires 2 parameters");
        return NULL;
    }

    yyjson_val *p0 = yyjson_arr_get(params, 0);
    yyjson_val *p1 = yyjson_arr_get(params, 1);

    if (!yyjson_is_num(p0) || !yyjson_is_num(p1)) {
        snprintf(err_buf, err_size, "multiply requires numeric parameters");
        return NULL;
    }

    double a = yyjson_get_num(p0);
    double b = yyjson_get_num(p1);
    double result = a * b;

    // Return as integer if both inputs were integers and result is whole
    if (yyjson_is_int(p0) && yyjson_is_int(p1)) {
        return yyjson_mut_int(doc, (int)result);
    }
    return yyjson_mut_real(doc, result);
}

// Example handler that accepts flexible parameter types
// Demonstrates: variable number of params, string handling, mixed types
yyjson_mut_val* greet_handler(yyjson_mut_doc *doc, yyjson_val *params, char *err_buf, size_t err_size) {
    if (!params || !yyjson_is_arr(params)) {
        snprintf(err_buf, err_size, "greet requires an array of parameters");
        return NULL;
    }

    size_t count = yyjson_arr_size(params);
    if (count < 1) {
        snprintf(err_buf, err_size, "greet requires at least 1 parameter (name)");
        return NULL;
    }

    // Get first param as name (can be string or convert number to string)
    yyjson_val *name_val = yyjson_arr_get(params, 0);
    char name_buf[128] = "Guest";

    if (yyjson_is_str(name_val)) {
        const char *name = yyjson_get_str(name_val);
        snprintf(name_buf, sizeof(name_buf), "%s", name);
    } else if (yyjson_is_num(name_val)) {
        snprintf(name_buf, sizeof(name_buf), "User%d", yyjson_get_int(name_val));
    }

    // Optional second param: age (number)
    int age = 0;
    if (count >= 2) {
        yyjson_val *age_val = yyjson_arr_get(params, 1);
        if (yyjson_is_num(age_val)) {
            age = yyjson_get_int(age_val);
        }
    }

    // Build result as JSON object
    yyjson_mut_val *result = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_str(doc, result, "greeting", "Hello");
    yyjson_mut_obj_add_str(doc, result, "name", name_buf);
    if (age > 0) {
        yyjson_mut_obj_add_int(doc, result, "age", age);
    }

    return result;
}

int main() {
    // Set up signal handler for Ctrl+C
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Create server
    RPCServer *server = rpc_server_new("test");
    if (!server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }

    // Register RPC functions
    rpc_server_register(server, "add", add_handler);
    rpc_server_register(server, "multiply", multiply_handler);
    rpc_server_register(server, "greet", greet_handler);  // Demo: flexible params

    // Start server
    if (rpc_server_start(server) != 0) {
        fprintf(stderr, "Failed to start server\n");
        rpc_server_free(server);
        return 1;
    }

    // Broadcast time updates every 2 seconds
    int update_count = 0;
    time_t last_publish = time(NULL);

    while (running) {
        sleep(1);

        time_t now = time(NULL);
        if (now - last_publish >= 2) {
            struct tm *tm_info = localtime(&now);
            char time_str[64];
            strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S%z", tm_info);

            char json_msg[256];
            snprintf(json_msg, sizeof(json_msg), "{\"value\":\"%s\"}", time_str);

            rpc_server_publish(server, "time", json_msg);

            update_count++;
            printf("Published time update %d\n", update_count);

            last_publish = now;
        }
    }

    // Clean up
    rpc_server_free(server);

    return 0;
}
