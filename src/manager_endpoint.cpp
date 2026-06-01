#include "manager_endpoint.h"

#include <ctype.h>
#include <stdlib.h>

namespace manager_endpoint {

namespace {

std::string trim_slashes(std::string path) {
    while (!path.empty() && path.back() == '/') {
        path.pop_back();
    }
    return path == "/" ? std::string() : path;
}

bool valid_port(const std::string &s, uint16_t &out) {
    if (s.empty()) return false;
    unsigned long v = 0;
    for (char c : s) {
        if (!isdigit(static_cast<unsigned char>(c))) return false;
        v = (v * 10UL) + static_cast<unsigned long>(c - '0');
        if (v > 65535UL) return false;
    }
    if (v == 0) return false;
    out = static_cast<uint16_t>(v);
    return true;
}

void set_error(std::string *error, const char *msg) {
    if (error) *error = msg ? msg : "";
}

}  // namespace

const char *discovery_method_to_string(DiscoveryMethod method) {
    switch (method) {
    case DiscoveryMethod::None:
        return "none";
    case DiscoveryMethod::Stored:
        return "stored";
    case DiscoveryMethod::Manual:
        return "manual";
    case DiscoveryMethod::Mdns:
        return "mdns";
    case DiscoveryMethod::SignalKWellKnown:
        return "signalk-well-known";
    }
    return "none";
}

DiscoveryMethod discovery_method_from_string(const char *value) {
    if (!value || !*value) return DiscoveryMethod::None;
    std::string s(value);
    if (s == "stored") return DiscoveryMethod::Stored;
    if (s == "manual") return DiscoveryMethod::Manual;
    if (s == "mdns") return DiscoveryMethod::Mdns;
    if (s == "signalk-well-known") return DiscoveryMethod::SignalKWellKnown;
    return DiscoveryMethod::None;
}

bool parse_url(const std::string &url, Endpoint &out, std::string *error) {
    if (url.empty()) {
        set_error(error, "empty url");
        return false;
    }

    Endpoint parsed;
    std::string rest = url;
    size_t scheme_pos = rest.find("://");
    if (scheme_pos != std::string::npos) {
        parsed.scheme = rest.substr(0, scheme_pos);
        rest = rest.substr(scheme_pos + 3);
    }

    if (parsed.scheme == "https") {
        parsed.tls = true;
        parsed.port = 443;
    } else if (parsed.scheme == "http") {
        parsed.tls = false;
        parsed.port = 80;
    } else {
        set_error(error, "unsupported scheme");
        return false;
    }

    size_t path_pos = rest.find('/');
    std::string authority = path_pos == std::string::npos ? rest : rest.substr(0, path_pos);
    parsed.base_path =
        path_pos == std::string::npos ? std::string() : trim_slashes(rest.substr(path_pos));
    if (authority.empty()) {
        set_error(error, "missing host");
        return false;
    }

    size_t port_pos = authority.rfind(':');
    if (port_pos != std::string::npos) {
        parsed.host = authority.substr(0, port_pos);
        uint16_t port = 0;
        if (!valid_port(authority.substr(port_pos + 1), port)) {
            set_error(error, "invalid port");
            return false;
        }
        parsed.port = port;
    } else {
        parsed.host = authority;
    }

    if (parsed.host.empty()) {
        set_error(error, "missing host");
        return false;
    }

    out = parsed;
    set_error(error, "");
    return true;
}

std::string base_url(const Endpoint &endpoint) {
    std::string out = endpoint.scheme + "://" + endpoint.host + ":" + std::to_string(endpoint.port);
    if (!endpoint.base_path.empty()) {
        if (endpoint.base_path[0] != '/') out += "/";
        out += endpoint.base_path;
    }
    return out;
}

std::string redacted_secret_state(const std::string &secret) {
    if (secret.empty()) return "none";
    return "set(len=" + std::to_string(secret.size()) + ")";
}

}  // namespace manager_endpoint
