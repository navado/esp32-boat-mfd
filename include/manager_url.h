#pragma once

// Pure-C++ URL helpers for the manager module.
//
// Extracted from src/manager.cpp so the path-joining and base-URL
// inference logic can be exercised on the host. The firmware-facing
// String overloads in manager.cpp delegate to these.

#include <string>

namespace manager_url {

// True iff the endpoint URL already has a path component after the
// authority (host:port). Used to decide whether to auto-append
// /plugins/espdisp-manager when the operator supplies a bare
// SignalK root URL.
//
// Examples:
//   endpoint_has_path("http://host:3000")            -> false
//   endpoint_has_path("http://host:3000/")           -> true
//   endpoint_has_path("http://host/plugins/x")       -> true
//   endpoint_has_path("host:3000")                   -> false
bool endpoint_has_path(const std::string &endpoint);

// If the endpoint already targets the espdisp-manager plugin, return
// it unchanged (with any trailing slash stripped). Otherwise append
// "/plugins/espdisp-manager" so the caller can probe both shapes.
std::string plugin_base_from_root(const std::string &endpoint);

// Join `base` and `path` with exactly one slash between them. Any
// trailing slash on `base` is removed; `path` is appended verbatim
// (so the caller controls whether it starts with "/").
std::string join_url(const std::string &base, const char *path);

}  // namespace manager_url
