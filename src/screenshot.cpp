#include "screenshot.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_heap_caps.h>
#include <lvgl.h>
#include <string.h>
#include <esp32s3/rom/miniz.h>

#include "net.h"

namespace screenshot {

struct Slot {
    SemaphoreHandle_t done;
    uint8_t *out_buf;
    size_t out_len;
    Format fmt;
    bool pending;
};

static Slot s_slot;
static SemaphoreHandle_t s_mutex;  // guards s_slot.pending

void setup() {
    s_slot = {};
    s_mutex = xSemaphoreCreateMutex();
}

#if LV_USE_SNAPSHOT

// --- Minimal PNG encoder using S3 ROM miniz ---------------------------------
//
// PNG containers a zlib-wrapped DEFLATE stream. miniz lives in ESP32-S3 ROM
// (esp32s3/rom/miniz.h) so we get DEFLATE + CRC32 + Adler32 for free, no
// flash cost. We only have to build the PNG chunk framing and convert
// RGB565 framebuffer rows to PNG-shape (1 filter byte + RGB888 pixels).
//
// 480x480 LVGL UI typically compresses to 20-60 KB (vs 460 kB BMP) which
// fits comfortably in a single TCP send window, avoiding the chunked-
// streaming dance entirely.

struct PngOut {
    uint8_t *buf;
    size_t cap;
    size_t len;
    bool oom;
};

static mz_bool tdefl_put_cb(const void *data, int len, void *user) {
    auto *o = (PngOut *)user;
    if (o->oom) return MZ_FALSE;
    if (o->len + (size_t)len > o->cap) {
        o->oom = true;
        return MZ_FALSE;
    }
    memcpy(o->buf + o->len, data, len);
    o->len += len;
    return MZ_TRUE;
}

static inline void put_u32_be(uint8_t *p, uint32_t v) {
    p[0] = (v >> 24) & 0xff;
    p[1] = (v >> 16) & 0xff;
    p[2] = (v >> 8) & 0xff;
    p[3] = v & 0xff;
}

// Append a PNG chunk (length + type + payload + CRC32) to buf at offset.
// Returns bytes written or 0 on overflow.
static size_t emit_chunk(uint8_t *buf, size_t buf_cap, size_t off, const char *type,
                         const uint8_t *payload, uint32_t payload_len) {
    size_t need = 4 + 4 + payload_len + 4;
    if (off + need > buf_cap) return 0;
    put_u32_be(buf + off, payload_len);
    memcpy(buf + off + 4, type, 4);
    if (payload_len) memcpy(buf + off + 8, payload, payload_len);
    uint32_t crc = mz_crc32(MZ_CRC32_INIT, buf + off + 4, 4 + payload_len);
    put_u32_be(buf + off + 8 + payload_len, crc);
    return need;
}

static bool build_png_from_snapshot(uint8_t **out, size_t *out_len) {
    lv_draw_buf_t *buf = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_NATIVE);
    if (!buf) return false;
    uint32_t w = buf->header.w;
    uint32_t h = buf->header.h;
    uint32_t stride = buf->header.stride;  // bytes per source row
    if (w == 0 || h == 0) {
        lv_draw_buf_destroy(buf);
        return false;
    }

    // Worst-case output budget: zlib-wrapped DEFLATE of (w*3+1)*h bytes.
    // Allocate generously - PSRAM is cheap (~7 MB free); we'll free
    // the actual compressed result back to its measured size.
    size_t raw_size = (size_t)(w * 3 + 1) * h;
    size_t zlib_cap = raw_size + raw_size / 200 + 64;  // ~0.5% overhead
    uint8_t *zlib_buf = (uint8_t *)heap_caps_malloc(zlib_cap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    tdefl_compressor *deflator = (tdefl_compressor *)heap_caps_malloc(
        sizeof(tdefl_compressor), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    uint8_t *row_buf = (uint8_t *)heap_caps_malloc(w * 3 + 1, MALLOC_CAP_8BIT);
    if (!zlib_buf || !deflator || !row_buf) {
        if (zlib_buf) heap_caps_free(zlib_buf);
        if (deflator) heap_caps_free(deflator);
        if (row_buf) heap_caps_free(row_buf);
        lv_draw_buf_destroy(buf);
        return false;
    }

    PngOut po = {zlib_buf, zlib_cap, 0, false};
    int flags = TDEFL_WRITE_ZLIB_HEADER | TDEFL_DEFAULT_MAX_PROBES;
    if (tdefl_init(deflator, tdefl_put_cb, &po, flags) != TDEFL_STATUS_OKAY) {
        heap_caps_free(zlib_buf);
        heap_caps_free(deflator);
        heap_caps_free(row_buf);
        lv_draw_buf_destroy(buf);
        return false;
    }

    // Feed each row through DEFLATE. Filter byte = 0 (None). Convert
    // RGB565 source pixels to RGB888 (PNG expects 8-bit samples).
    const uint8_t *src = (const uint8_t *)buf->data;
    for (uint32_t y = 0; y < h; ++y) {
        row_buf[0] = 0;  // filter = None
        const uint8_t *row = src + (size_t)y * stride;
        for (uint32_t x = 0; x < w; ++x) {
            uint16_t px = (uint16_t)row[2 * x] | ((uint16_t)row[2 * x + 1] << 8);
            uint8_t r5 = (px >> 11) & 0x1f;
            uint8_t g6 = (px >> 5) & 0x3f;
            uint8_t b5 = px & 0x1f;
            row_buf[1 + x * 3 + 0] = (r5 << 3) | (r5 >> 2);
            row_buf[1 + x * 3 + 1] = (g6 << 2) | (g6 >> 4);
            row_buf[1 + x * 3 + 2] = (b5 << 3) | (b5 >> 2);
        }
        if (tdefl_compress_buffer(deflator, row_buf, w * 3 + 1, TDEFL_NO_FLUSH) !=
            TDEFL_STATUS_OKAY) {
            heap_caps_free(zlib_buf);
            heap_caps_free(deflator);
            heap_caps_free(row_buf);
            lv_draw_buf_destroy(buf);
            return false;
        }
    }
    if (tdefl_compress_buffer(deflator, nullptr, 0, TDEFL_FINISH) != TDEFL_STATUS_DONE) {
        heap_caps_free(zlib_buf);
        heap_caps_free(deflator);
        heap_caps_free(row_buf);
        lv_draw_buf_destroy(buf);
        return false;
    }
    heap_caps_free(deflator);
    heap_caps_free(row_buf);
    lv_draw_buf_destroy(buf);
    if (po.oom) {
        heap_caps_free(zlib_buf);
        return false;
    }

    // Assemble PNG container: signature + IHDR + IDAT + IEND.
    size_t png_cap = 8 + (4 + 4 + 13 + 4) + (4 + 4 + po.len + 4) + (4 + 4 + 0 + 4);
    uint8_t *png = (uint8_t *)heap_caps_malloc(png_cap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!png) {
        heap_caps_free(zlib_buf);
        return false;
    }
    static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
    memcpy(png, sig, 8);
    size_t off = 8;

    uint8_t ihdr[13];
    put_u32_be(ihdr + 0, w);
    put_u32_be(ihdr + 4, h);
    ihdr[8] = 8;   // bit depth
    ihdr[9] = 2;   // color type: RGB
    ihdr[10] = 0;  // compression
    ihdr[11] = 0;  // filter
    ihdr[12] = 0;  // interlace
    size_t n = emit_chunk(png, png_cap, off, "IHDR", ihdr, 13);
    if (!n) {
        heap_caps_free(zlib_buf);
        heap_caps_free(png);
        return false;
    }
    off += n;

    n = emit_chunk(png, png_cap, off, "IDAT", zlib_buf, po.len);
    heap_caps_free(zlib_buf);
    if (!n) {
        heap_caps_free(png);
        return false;
    }
    off += n;

    n = emit_chunk(png, png_cap, off, "IEND", nullptr, 0);
    if (!n) {
        heap_caps_free(png);
        return false;
    }
    off += n;

    *out = png;
    *out_len = off;
    return true;
}

static bool build_bmp_from_snapshot(uint8_t **out, size_t *out_len) {
    lv_draw_buf_t *buf = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_NATIVE);
    if (!buf) return false;
    uint32_t w = buf->header.w;
    uint32_t h = buf->header.h;
    uint32_t stride = buf->header.stride;
    uint32_t pix_bytes = stride * h;
    uint32_t total = 14 + 40 + 12 + pix_bytes;

    uint8_t *bmp = (uint8_t *)heap_caps_malloc(total, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!bmp) {
        lv_draw_buf_destroy(buf);
        return false;
    }
    memset(bmp, 0, 14 + 40 + 12);
    // BMP file header
    bmp[0] = 'B';
    bmp[1] = 'M';
    bmp[2] = total & 0xff;
    bmp[3] = (total >> 8) & 0xff;
    bmp[4] = (total >> 16) & 0xff;
    bmp[5] = (total >> 24) & 0xff;
    bmp[10] = (14 + 40 + 12) & 0xff;  // pixel data offset
    // DIB header (40 bytes)
    bmp[14] = 40;
    bmp[18] = w & 0xff;
    bmp[19] = (w >> 8) & 0xff;
    int32_t neg_h = -(int32_t)h;
    bmp[22] = neg_h & 0xff;
    bmp[23] = (neg_h >> 8) & 0xff;
    bmp[24] = (neg_h >> 16) & 0xff;
    bmp[25] = (neg_h >> 24) & 0xff;
    bmp[26] = 1;   // planes
    bmp[28] = 16;  // bpp
    bmp[30] = 3;   // BI_BITFIELDS
    bmp[34] = pix_bytes & 0xff;
    bmp[35] = (pix_bytes >> 8) & 0xff;
    bmp[36] = (pix_bytes >> 16) & 0xff;
    bmp[37] = (pix_bytes >> 24) & 0xff;
    // RGB565 channel masks
    bmp[54] = 0x00;
    bmp[55] = 0xF8;
    bmp[58] = 0xE0;
    bmp[59] = 0x07;
    bmp[62] = 0x1F;

    memcpy(bmp + 14 + 40 + 12, buf->data, pix_bytes);
    lv_draw_buf_destroy(buf);
    *out = bmp;
    *out_len = total;
    return true;
}
#endif

void serve_pending() {
    if (!s_mutex || !xSemaphoreTake(s_mutex, 0)) return;
    bool pending = s_slot.pending;
    Format fmt = s_slot.fmt;
    xSemaphoreGive(s_mutex);
    if (!pending) return;

#if LV_USE_SNAPSHOT
    uint8_t *out = nullptr;
    size_t len = 0;
    bool ok = (fmt == Format::Png) ? build_png_from_snapshot(&out, &len)
                                   : build_bmp_from_snapshot(&out, &len);
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_slot.out_buf = ok ? out : nullptr;
    s_slot.out_len = ok ? len : 0;
    s_slot.pending = false;
    xSemaphoreGive(s_mutex);
#else
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_slot.out_buf = nullptr;
    s_slot.out_len = 0;
    s_slot.pending = false;
    xSemaphoreGive(s_mutex);
#endif
    if (s_slot.done) xSemaphoreGive(s_slot.done);
}

bool request(uint32_t timeout_ms, uint8_t **out, size_t *out_len, Format fmt) {
    if (!s_mutex) return false;
    if (!xSemaphoreTake(s_mutex, pdMS_TO_TICKS(timeout_ms))) return false;
    if (s_slot.pending) {
        // Someone else is mid-flight - too slow to queue, just fail.
        xSemaphoreGive(s_mutex);
        return false;
    }
    s_slot.done = xSemaphoreCreateBinary();
    s_slot.out_buf = nullptr;
    s_slot.out_len = 0;
    s_slot.fmt = fmt;
    s_slot.pending = true;
    xSemaphoreGive(s_mutex);

    bool got = xSemaphoreTake(s_slot.done, pdMS_TO_TICKS(timeout_ms));
    SemaphoreHandle_t sem = s_slot.done;
    s_slot.done = nullptr;
    if (sem) vSemaphoreDelete(sem);

    if (!got) return false;
    *out = s_slot.out_buf;
    *out_len = s_slot.out_len;
    return *out != nullptr;
}

}  // namespace screenshot
