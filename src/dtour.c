#include <arpa/inet.h>
#include <errno.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void read_cb(struct bufferevent *bev, void *ctx);
static void event_cb(struct bufferevent *bev, short events, void *ctx);

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s -from IP:port -to IP:port\n", argv[0]);
        return 1;
    }

    char *src_addr = NULL, *dst_addr = NULL;
    unsigned short src_port = 0, dst_port = 0;

    for (int i = 1; i < argc; i += 2)
    {
        if (strcmp(argv[i], "-from") == 0)
        {
            char *sep = strchr(argv[i + 1], ':');
            if (!sep)
            {
                fprintf(stderr, "Invalid source address format.\n");
                return 1;
            }
            *sep = '\0';
            src_addr = argv[i + 1];
            src_port = (unsigned short)atoi(sep + 1);
        }
        else if (strcmp(argv[i], "-to") == 0)
        {
            char *sep = strchr(argv[i + 1], ':');
            if (!sep)
            {
                fprintf(stderr, "Invalid destination address format.\n");
                return 1;
            }
            *sep = '\0';
            dst_addr = argv[i + 1];
            dst_port = (unsigned short)atoi(sep + 1);
        }
    }

    if (!src_addr || !dst_addr || !src_port || !dst_port)
    {
        fprintf(stderr, "Invalid arguments.\n");
        return 1;
    }

    struct event_base *base = event_base_new();
    if (!base)
    {
        perror("event_base_new()");
        return 1;
    }

    struct sockaddr_in src_sin;
    memset(&src_sin, 0, sizeof(src_sin));
    src_sin.sin_family = AF_INET;
    src_sin.sin_port = htons(src_port);
    inet_pton(AF_INET, src_addr, &src_sin.sin_addr);

    struct sockaddr_in dst_sin;
    memset(&dst_sin, 0, sizeof(dst_sin));
    dst_sin.sin_family = AF_INET;
    dst_sin.sin_port = htons(dst_port);
    inet_pton(AF_INET, dst_addr, &dst_sin.sin_addr);

    struct bufferevent *src_bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (!src_bev)
    {
        perror("bufferevent_socket_new()");
        return 1;
    }

    bufferevent_setcb(src_bev, read_cb, NULL, event_cb, &dst_sin);
    bufferevent_enable(src_bev, EV_READ | EV_WRITE);

    if (bufferevent_socket_connect(src_bev, (struct sockaddr *)&src_sin, sizeof(src_sin)) < 0)
    {
                perror("bufferevent_socket_connect()");
        bufferevent_free(src_bev);
        return 1;
    }

    printf("Redirecting traffic from %s:%d to %s:%d\n", src_addr, src_port, dst_addr, dst_port);

    event_base_dispatch(base);

    bufferevent_free(src_bev);
    event_base_free(base);

    return 0;
}

static void read_cb(struct bufferevent *bev, void *ctx)
{
    struct sockaddr_in *dst_sin = (struct sockaddr_in *)ctx;
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);

    struct bufferevent *dst_bev = bufferevent_socket_new(bufferevent_get_base(bev), -1, BEV_OPT_CLOSE_ON_FREE);

    if (!dst_bev)
    {
        perror("bufferevent_socket_new()");
        return;
    }

    bufferevent_setcb(dst_bev, NULL, NULL, event_cb, NULL);
    bufferevent_enable(dst_bev, EV_READ | EV_WRITE);

    if (bufferevent_socket_connect(dst_bev, (struct sockaddr *)dst_sin, sizeof(*dst_sin)) < 0)
    {
        perror("bufferevent_socket_connect()");
        bufferevent_free(dst_bev);
        return;
    }

    evbuffer_add_buffer(output, input);
    bufferevent_write_buffer(dst_bev, output);

    bufferevent_free(dst_bev);
}

static void event_cb(struct bufferevent *bev, short events, void *ctx)
{
    if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF))
    {
        bufferevent_free(bev);
    }
}

