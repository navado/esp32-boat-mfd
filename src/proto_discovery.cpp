// mDNS browse for espdisp displays. Wraps the ESP-IDF `mdns` component
// (already linked; Arduino's MDNS wraps the same instance). The query is
// blocking, so callers run it on the worker/network task.
#include "proto_discovery.h"

#include <mdns.h>
#include <string.h>

namespace proto_discovery {

namespace {

// Copy a NUL-terminated source into a fixed dest, always terminated.
void copy_str(char *dst, size_t cap, const char *src) {
    if (!cap) return;
    if (!src) src = "";
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

// Pull a TXT value by key into `dst`. The mDNS result stores TXT items as a
// parallel key/value array. No-op (leaves dst as-is) when the key is absent.
void txt_get(const mdns_result_t *r, const char *key, char *dst, size_t cap) {
    for (size_t i = 0; i < r->txt_count; ++i) {
        if (r->txt[i].key && strcmp(r->txt[i].key, key) == 0) {
            copy_str(dst, cap, r->txt[i].value ? r->txt[i].value : "");
            return;
        }
    }
}

}  // namespace

int browse(PeerCallback cb, void *ctx, uint32_t timeout_ms, size_t max_results) {
    if (!cb) return -1;
    mdns_result_t *results = nullptr;
    // Service type + proto are passed WITHOUT the leading underscores; the
    // component prepends them. Matches the registration in net.cpp which uses
    // device_discovery::MDNS_SERVICE ("espdisp") + "tcp".
    esp_err_t err = mdns_query_ptr("_espdisp", "_tcp", timeout_ms, max_results, &results);
    if (err != ESP_OK) return -1;

    int n = 0;
    for (mdns_result_t *r = results; r != nullptr; r = r->next) {
        Peer p;  // small POD, fits the worker stack
        txt_get(r, "device_id", p.device_id, sizeof(p.device_id));
        txt_get(r, "board", p.board, sizeof(p.board));
        txt_get(r, "display", p.display, sizeof(p.display));
        txt_get(r, "pv", p.pv, sizeof(p.pv));
        txt_get(r, "role", p.role, sizeof(p.role));
        p.port = r->port;

        // First IPv4 address wins. The addr list can carry v4 + v6; we only
        // build an HTTP base for v4 here (IP-primary transport).
        for (mdns_ip_addr_t *a = r->addr; a != nullptr; a = a->next) {
            if (a->addr.type == ESP_IPADDR_TYPE_V4) {
                snprintf(p.ip, sizeof(p.ip), IPSTR, IP2STR(&a->addr.u_addr.ip4));
                break;
            }
        }
        if (!p.ip[0]) continue;  // no usable IPv4 -> skip (can't build a base URL)
        if (p.port == 0) p.port = 80;
        snprintf(p.base_url, sizeof(p.base_url), "http://%s:%u", p.ip, (unsigned)p.port);

        cb(p, ctx);
        ++n;
    }
    mdns_query_results_free(results);
    return n;
}

}  // namespace proto_discovery
