#include "device_discovery.h"

namespace device_discovery {

void build_announcement(JsonDocument &doc, const Info &info) {
    doc.clear();
    doc["protocol"] = DEVICE_ANNOUNCE_PROTOCOL;
    doc["deviceId"] = info.device_id;
    doc["name"] = info.device_id;
    doc["address"] = info.ip;
    doc["port"] = info.port;
    doc["path"] = "/";
    doc["authRequired"] = info.web_auth_required;

    JsonObject device = doc["device"].to<JsonObject>();
    device["id"] = info.device_id;
    device["board"] = info.board_id;

    JsonObject firmware = doc["firmware"].to<JsonObject>();
    firmware["name"] = info.firmware_name;
    firmware["version"] = info.firmware_version;

    JsonObject display = doc["display"].to<JsonObject>();
    display["width"] = info.display_width;
    display["height"] = info.display_height;
}

}  // namespace device_discovery
