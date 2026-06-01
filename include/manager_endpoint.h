#pragma once

// Pure-C++ manager endpoint model.
//
// The firmware manager client persists and reports one canonical endpoint
// shape while discovery/register/config code may receive several URL forms.
// Keeping this parser host-testable prevents CLI, mDNS, and well-known
// discovery paths from inventing subtly different interpretations.

#include <stdint.h>

#include <string>

namespace manager_endpoint {

enum class DiscoveryMethod : uint8_t {
    None = 0,
    Stored,
    Manual,
    Mdns,
    SignalKWellKnown,
};

struct Endpoint {
    std::string scheme = "http";
    std::string host;
    uint16_t port = 80;
    std::string base_path;
    bool tls = false;
    DiscoveryMethod discovery = DiscoveryMethod::None;
};

const char *discovery_method_to_string(DiscoveryMethod method);
DiscoveryMethod discovery_method_from_string(const char *value);

// Parse http(s)://host[:port][/path]. Missing scheme defaults to http.
// Missing port defaults to 80 for http and 443 for https. Empty/root path
// becomes an empty base_path.
bool parse_url(const std::string &url, Endpoint &out, std::string *error = nullptr);

std::string base_url(const Endpoint &endpoint);
std::string redacted_secret_state(const std::string &secret);

}  // namespace manager_endpoint
