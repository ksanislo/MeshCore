/*
 * fs_shim.cpp — Arduino fs::FS / fs::File over ESP-IDF VFS + POSIX stdio.
 * See FS.h for scope. Implements the FS methods and the SPIFFS mount helper.
 */
#include "FS.h"

#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "esp_spiffs.h"
#include "esp_log.h"

static const char* TAG = "fs_shim";

namespace fs {

// Global mounts. Rooted at the SPIFFS VFS mount point / partition label.
FS InternalFS("/spiffs", "spiffs");
FS SPIFFS("/spiffs", "spiffs");

// Translate an Arduino open-mode string to a POSIX (binary) fopen mode.
static const char* translateMode(const char* mode) {
  if (mode == nullptr)            return "rb";
  // Longest/compound modes first.
  if (!strcmp(mode, "r+"))        return "rb+";
  if (!strcmp(mode, "w+"))        return "wb+";
  if (!strcmp(mode, "a+"))        return "ab+";
  if (!strcmp(mode, "rb+"))       return "rb+";
  if (!strcmp(mode, "wb+"))       return "wb+";
  if (!strcmp(mode, "ab+"))       return "ab+";
  if (!strcmp(mode, "w"))         return "wb";
  if (!strcmp(mode, "a"))         return "ab";
  if (!strcmp(mode, "r"))         return "rb";
  // Already-binary or unknown: pass through (fopen validates).
  return mode;
}

// Join the mount root with an absolute in-mount path ("/foo" -> "/spiffs/foo").
static std::string joinPath(const std::string& root, const char* path) {
  std::string p = path ? path : "";
  if (root.empty()) return p;
  if (!p.empty() && p[0] != '/') return root + "/" + p;
  return root + p;
}

File FS::open(const char* path, const char* mode, bool /*create*/) {
  std::string full = joinPath(_root, path);
  FILE* fp = fopen(full.c_str(), translateMode(mode));
  if (!fp) return File();
  return File(fp, path);
}

bool FS::exists(const char* path) {
  std::string full = joinPath(_root, path);
  struct stat st;
  return stat(full.c_str(), &st) == 0;
}

bool FS::remove(const char* path) {
  std::string full = joinPath(_root, path);
  return ::remove(full.c_str()) == 0;
}

bool FS::mkdir(const char* path) {
  std::string full = joinPath(_root, path);
  int r = ::mkdir(full.c_str(), 0777);
  // SPIFFS is flat and has no real directories; treat "unsupported"/"exists"
  // as success so callers that pre-create dirs (IdentityStore, "/bl") proceed.
  return r == 0 || errno == EEXIST || errno == ENOTSUP || errno == ENOSYS;
}

bool FS::rmdir(const char* path) {
  std::string full = joinPath(_root, path);
  return ::rmdir(full.c_str()) == 0;
}

bool FS::rename(const char* from, const char* to) {
  std::string a = joinPath(_root, from);
  std::string b = joinPath(_root, to);
  return ::rename(a.c_str(), b.c_str()) == 0;
}

bool FS::format() {
  if (_label.empty()) return false;
  return esp_spiffs_format(_label.c_str()) == ESP_OK;
}

size_t FS::totalBytes() {
  if (_label.empty()) return 0;
  size_t total = 0, used = 0;
  if (esp_spiffs_info(_label.c_str(), &total, &used) != ESP_OK) return 0;
  return total;
}

size_t FS::usedBytes() {
  if (_label.empty()) return 0;
  size_t total = 0, used = 0;
  if (esp_spiffs_info(_label.c_str(), &total, &used) != ESP_OK) return 0;
  return used;
}

} // namespace fs

bool fs_mount_spiffs() {
  esp_vfs_spiffs_conf_t conf = {};
  conf.base_path = "/spiffs";
  conf.partition_label = "spiffs";
  conf.max_files = 8;
  conf.format_if_mount_failed = true;

  esp_err_t err = esp_vfs_spiffs_register(&conf);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(err));
    return false;
  }
  size_t total = 0, used = 0;
  if (esp_spiffs_info(conf.partition_label, &total, &used) == ESP_OK) {
    ESP_LOGI(TAG, "SPIFFS mounted: %u/%u bytes used", (unsigned)used, (unsigned)total);
  }
  return true;
}
