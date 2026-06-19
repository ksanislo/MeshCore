#include "RingtonePack.h"
#ifdef HAS_SD_CARD

#include "SdCard.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#ifndef OTA_GH_OWNER
  #define OTA_GH_OWNER "ksanislo"
#endif
#ifndef OTA_GH_REPO
  #define OTA_GH_REPO "MeshCore-LVGL"
#endif

namespace RingtonePack {

static volatile bool s_busy   = false;
static volatile bool s_cancel = false;
static char s_status[64]      = "";

bool busy()  { return s_busy; }
void cancel() { if (s_busy) s_cancel = true; }
void status(char* out, size_t cap) {
  if (!out || !cap) return;
  strncpy(out, s_status[0] ? s_status : "tap to download", cap - 1);
  out[cap - 1] = '\0';
}
static void setStatus(const char* m) {
  strncpy(s_status, m, sizeof(s_status) - 1);
  s_status[sizeof(s_status) - 1] = '\0';
}

// Parse pack.txt line-by-line from a heap buffer.
// Each non-blank, non-comment line is a full RTTTL string: "Name:d=...:...."
// We write each one to /ringtones/<Name>.rtttl on the SD card.
static int writeRingtones(const char* buf, size_t len) {
  int written = 0;
  const char* p = buf;
  const char* end = buf + len;

  // Ensure /ringtones/ directory exists
  {
    SdSvc::Lock lk;
    if (!sd.exists("/ringtones")) sd.mkdir("/ringtones");
  }

  while (p < end) {
    // Find end of line
    const char* nl = (const char*)memchr(p, '\n', end - p);
    size_t linelen = nl ? (size_t)(nl - p) : (size_t)(end - p);

    // Trim trailing \r
    const char* lineend = p + linelen;
    while (lineend > p && *(lineend - 1) == '\r') lineend--;
    size_t trimlen = (size_t)(lineend - p);

    if (trimlen > 0 && p[0] != '#') {
      // Extract name (up to first ':')
      const char* colon = (const char*)memchr(p, ':', trimlen);
      if (colon && colon > p) {
        char name[32];
        size_t namelen = (size_t)(colon - p);
        if (namelen > sizeof(name) - 1) namelen = sizeof(name) - 1;
        memcpy(name, p, namelen);
        name[namelen] = '\0';

        // Write RTTTL string to /ringtones/<Name>.rtttl
        char path[48];
        snprintf(path, sizeof(path), "/ringtones/%s.rtttl", name);

        {
          SdSvc::Lock lk;
          FsFile f = sd.open(path, O_WRONLY | O_CREAT | O_TRUNC);
          if (f) {
            f.write(p, trimlen);
            f.close();
            written++;
          }
        }

        char sb[48];
        snprintf(sb, sizeof(sb), "saved %s (%d)", name, written);
        setStatus(sb);
      }
    }

    p = nl ? nl + 1 : end;
  }
  return written;
}

static void downloadTask(void*) {
  s_cancel = false;

  char url[200];
  snprintf(url, sizeof(url),
           "https://raw.githubusercontent.com/%s/%s/main/ringtones/pack.txt",
           OTA_GH_OWNER, OTA_GH_REPO);

  setStatus("resolving...");
  IPAddress rip;
  if (!WiFi.hostByName("raw.githubusercontent.com", rip)) {
    setStatus("DNS failed");
    s_busy = false;
    vTaskDelete(nullptr);
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(15000);
  http.setConnectTimeout(8000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(client, url)) {
    setStatus("connect failed");
    s_busy = false;
    vTaskDelete(nullptr);
    return;
  }
  http.addHeader("User-Agent", "MeshCore-LVGL-RingtonePack");
  setStatus("downloading...");
  int code = http.GET();
  if (code != 200) {
    char b[40];
    snprintf(b, sizeof(b), "HTTP %d", code);
    setStatus(b);
    http.end();
    s_busy = false;
    vTaskDelete(nullptr);
    return;
  }

  // Read the whole pack.txt into heap (it's tiny — a few KB)
  int contentLen = http.getSize();
  size_t cap = (contentLen > 0 && contentLen < 65536) ? (size_t)contentLen + 4 : 65536;
  char* buf = (char*)malloc(cap);
  if (!buf) {
    setStatus("out of memory");
    http.end();
    s_busy = false;
    vTaskDelete(nullptr);
    return;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t got = 0;
  uint32_t last = millis();
  while (got < cap - 1 && !s_cancel) {
    size_t avail = stream->available();
    if (avail) {
      int r = stream->readBytes(buf + got, avail > (cap - 1 - got) ? cap - 1 - got : avail);
      if (r > 0) { got += (size_t)r; last = millis(); }
    } else if (!http.connected()) {
      break;
    } else if (millis() - last > 10000) {
      break;
    } else {
      delay(2);
    }
  }
  http.end();

  if (s_cancel) {
    free(buf);
    setStatus("cancelled");
    s_busy = false;
    vTaskDelete(nullptr);
    return;
  }

  buf[got] = '\0';
  int n = writeRingtones(buf, got);
  free(buf);

  char done[48];
  snprintf(done, sizeof(done), "done - %d ringtones saved", n);
  setStatus(done);
  s_busy = false;
  vTaskDelete(nullptr);
}

void start() {
  if (s_busy) return;
  s_busy = true;
  setStatus("starting...");
  xTaskCreatePinnedToCore(downloadTask, "rtdl", 8192, nullptr, 1, nullptr, 1);
}

} // namespace RingtonePack

#endif // HAS_SD_CARD
