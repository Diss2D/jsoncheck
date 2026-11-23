#define _POSIX_C_SOURCE 200809L
#include "server.h"
#include "core.h"
#include <microhttpd.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_JSON_SIZE (1024U * 1024U)

struct RequestBody {
    char *data;
    size_t length;
    int too_large;
    int has_null_byte;
};

static void request_completed(void *cls,
                              struct MHD_Connection *connection,
                              void **con_cls,
                              enum MHD_RequestTerminationCode reason) {
    (void)cls;
    (void)connection;
    (void)reason;

    struct RequestBody *body = *con_cls;

    if (body != NULL) {
        free(body->data);
        free(body);
        *con_cls = NULL;
    }
}

static enum MHD_Result
send_response(struct MHD_Connection *connection, unsigned int status, const char *body) {
    struct MHD_Response *response =
        MHD_create_response_from_buffer(strlen(body), (void *)body, MHD_RESPMEM_MUST_COPY);

    if (response == NULL) {
        return MHD_NO;
    }
    if (MHD_add_response_header(
            response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain; charset=utf-8") != MHD_YES) {
        MHD_destroy_response(response);
        return MHD_NO;
    }

    enum MHD_Result result = MHD_queue_response(connection, status, response);

    MHD_destroy_response(response);
    return result;
}

static enum MHD_Result send_json_response(struct MHD_Connection *connection, const char *body) {
    struct MHD_Response *response =
        MHD_create_response_from_buffer(strlen(body), (void *)body, MHD_RESPMEM_MUST_COPY);

    if (response == NULL) {
        return MHD_NO;
    }

    if (MHD_add_response_header(
            response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json; charset=utf-8") != MHD_YES) {
        MHD_destroy_response(response);
        return MHD_NO;
    }

    enum MHD_Result result = MHD_queue_response(connection, MHD_HTTP_OK, response);

    MHD_destroy_response(response);
    return result;
}

static enum MHD_Result handle_json_api(struct MHD_Connection *connection,
                                       const char *method,
                                       const char *upload_data,
                                       size_t *upload_data_size,
                                       void **con_cls,
                                       bool format) {
    if (strcmp(method, "POST") != 0) {
        return send_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, "Method not allowed\n");
    }

    const char *expected_token = getenv("APP_API_TOKEN");

    if (expected_token == NULL || expected_token[0] == '\0') {
        return send_response(
            connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "API token is not configured\n");
    }

    const char *provided_token =
        MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Api-Token");

    if (provided_token == NULL || strcmp(provided_token, expected_token) != 0) {
        return send_response(connection, MHD_HTTP_UNAUTHORIZED, "Unauthorized\n");
    }
    bool pretty = true;

    if (format) {
        const char *mode = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "mode");

        if (mode != NULL) {
            if (strcmp(mode, "compact") == 0) {
                pretty = false;
            } else if (strcmp(mode, "pretty") != 0) {
                return send_response(connection, MHD_HTTP_BAD_REQUEST, "Invalid format mode\n");
            }
        }
    }

    if (*con_cls == NULL) {
        struct RequestBody *body = calloc(1, sizeof(*body));

        if (body == NULL) {
            return MHD_NO;
        }

        *con_cls = body;
        return MHD_YES;
    }

    struct RequestBody *body = *con_cls;

    if (*upload_data_size > 0) {
        size_t chunk_size = *upload_data_size;

        if (chunk_size > MAX_JSON_SIZE - body->length) {
            body->too_large = 1;
        } else if (!body->too_large && !body->has_null_byte) {
            if (memchr(upload_data, '\0', chunk_size) != NULL) {
                body->has_null_byte = 1;
            } else {
                char *new_data = realloc(body->data, body->length + chunk_size + 1);

                if (new_data == NULL) {
                    return MHD_NO;
                }

                body->data = new_data;

                memcpy(body->data + body->length, upload_data, chunk_size);

                body->length += chunk_size;
                body->data[body->length] = '\0';
            }
        }

        *upload_data_size = 0;
        return MHD_YES;
    }

    if (body->too_large) {
        return send_response(connection, 413, "Request body too large\n");
    }

    if (body->has_null_byte || body->length == 0) {
        return send_response(connection, MHD_HTTP_BAD_REQUEST, "Invalid request body\n");
    }

    size_t error_offset = 0;

    if (!jsoncheck_validate(body->data, &error_offset)) {
        size_t line = 1;
        size_t column = 1;

        jsoncheck_error_location(body->data, error_offset, &line, &column);

        char message[160];

        snprintf(message, sizeof(message), "Invalid JSON at line %zu, column %zu\n", line, column);

        return send_response(connection, MHD_HTTP_BAD_REQUEST, message);
    }

    if (!format) {
        return send_response(connection, MHD_HTTP_OK, "JSON is valid\n");
    }

    char *formatted = jsoncheck_format(body->data, pretty);

    if (formatted == NULL) {
        return send_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Cannot format JSON\n");
    }

    enum MHD_Result result = send_json_response(connection, formatted);

    jsoncheck_free(formatted);

    return result;
}

static enum MHD_Result handle_request(void *cls,
                                      struct MHD_Connection *connection,
                                      const char *url,
                                      const char *method,
                                      const char *http_version,
                                      const char *upload_data,
                                      size_t *upload_data_size,
                                      void **con_cls) {
    (void)http_version;

    if (strcmp(url, "/api/validate") == 0 || strcmp(url, "/api/format") == 0) {
        return handle_json_api(connection,
                               method,
                               upload_data,
                               upload_data_size,
                               con_cls,
                               strcmp(url, "/api/format") == 0);
    }
    if (strcmp(method, "GET") != 0) {
        return send_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, "Method not allowed\n");
    }
    if (strcmp(url, "/health") == 0) {
        return send_response(connection, MHD_HTTP_OK, "OK\n");
    }
    if (strcmp(url, "/ready") == 0) {
        return send_response(connection, MHD_HTTP_OK, "READY\n");
    }
    if (strcmp(url, "/version") == 0) {
        const char *app_version = cls;

        char hostname[128] = "unknown";
        char body[256];
        if (gethostname(hostname, sizeof(hostname)) != 0) {
            strcpy(hostname, "unknown");
        }
        hostname[sizeof(hostname) - 1] = '\0';

        snprintf(body, sizeof(body), "version=%s hostname=%s\n", app_version, hostname);

        return send_response(connection, MHD_HTTP_OK, body);
    }

    return send_response(connection, MHD_HTTP_NOT_FOUND, "Not found\n");
}

int jsoncheck_server_run(const char *version) {
    unsigned int port = 8080;
    const char *port_env = getenv("APP_PORT");

    if (port_env != NULL) {
        char *end = NULL;
        errno = 0;

        unsigned long value = strtoul(port_env, &end, 10);

        if (errno != 0 || end == port_env || *end != '\0' || value == 0 || value > 65535) {
            fprintf(stderr, "Invalid APP_PORT\n");
            return 1;
        }
        port = (unsigned int)value;
    }

    sigset_t signals;
    sigset_t old_mask;

    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &signals, &old_mask) != 0) {
        perror("sigprocmask");
        return 1;
    }
    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD,
                                                 port,
                                                 NULL,
                                                 NULL,
                                                 handle_request,
                                                 (void *)version,
                                                 MHD_OPTION_NOTIFY_COMPLETED,
                                                 request_completed,
                                                 NULL,
                                                 MHD_OPTION_END);

    if (daemon == NULL) {
        fprintf(stderr, "Cannot start HTTP server\n");
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
        return 1;
    }

    printf("jsoncheck HTTP server listening on port %u\n", port);
    fflush(stdout);

    int received_signal;
    int result = sigwait(&signals, &received_signal);

    MHD_stop_daemon(daemon);
    sigprocmask(SIG_SETMASK, &old_mask, NULL);

    if (result != 0) {
        fprintf(stderr, "Signal wait failed\n");
        return 1;
    }

    return 0;
}
