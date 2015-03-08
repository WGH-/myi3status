#pragma once

extern "C" {
    int nl_get_multicast_id(struct nl_sock *sock, const char *family, const char *group);
}
