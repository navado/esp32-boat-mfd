#pragma once
// Host shim for <nvs.h>: storage.h only needs the nvs_handle_t type to
// declare its member; the harness stubs storage::Namespace's methods (no
// persistence on host).
#include <stdint.h>
typedef uint32_t nvs_handle_t;
