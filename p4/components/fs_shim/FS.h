/*
 * FS.h — Arduino <FS.h> (fs::FS / fs::File) compatibility shim for ESP-IDF.
 *
 * Backs the Arduino filesystem surface that vendored MeshCore code
 * (helpers/IdentityStore.{h,cpp}, examples/companion_radio/DataStore.{h,cpp})
 * expects, implemented over the ESP-IDF VFS + POSIX stdio (fopen/fread/...).
 * It mirrors only the slice of Arduino-esp32's fs::FS / fs::File API that
 * DataStore/IdentityStore actually use — this is deliberately NOT a complete
 * Arduino FS implementation.
 *
 * File is a value type (copyable/movable) that reference-counts the underlying
 * FILE* so an explicit close() and later destructor never double-close. File
 * derives from Stream because MeshCore's Identity::readFrom/writeTo take a
 * Stream&.
 */
#pragma once

#include <Stream.h>   // MeshCore's Arduino Stream stub (base class for File)

#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>

namespace fs {

// Owns the FILE*; closed once on last reference (or explicit File::close()).
struct FileImpl {
  FILE* fp = nullptr;
  std::string name;
  ~FileImpl() { if (fp) fclose(fp); }
};

class File : public Stream {
  std::shared_ptr<FileImpl> _impl;
public:
  File() = default;
  File(FILE* fp, const char* name) {
    if (fp) {
      _impl = std::make_shared<FileImpl>();
      _impl->fp = fp;
      _impl->name = name ? name : "";
    }
  }

  // ---- Stream overrides (byte-oriented) ----
  int available() override {
    if (!_impl || !_impl->fp) return 0;
    long cur = ftell(_impl->fp);
    if (cur < 0) return 0;
    fseek(_impl->fp, 0, SEEK_END);
    long end = ftell(_impl->fp);
    fseek(_impl->fp, cur, SEEK_SET);
    return end > cur ? (int)(end - cur) : 0;
  }
  int read() override {
    if (!_impl || !_impl->fp) return -1;
    return fgetc(_impl->fp);            // EOF == -1
  }
  int peek() override {
    if (!_impl || !_impl->fp) return -1;
    int c = fgetc(_impl->fp);
    if (c != EOF) ungetc(c, _impl->fp);
    return c;
  }
  size_t write(uint8_t b) override {
    if (!_impl || !_impl->fp) return 0;
    return fwrite(&b, 1, 1, _impl->fp);
  }
  size_t write(const uint8_t* buf, size_t len) override {
    if (!_impl || !_impl->fp) return 0;
    return fwrite(buf, 1, len, _impl->fp);
  }
  void flush() override { if (_impl && _impl->fp) fflush(_impl->fp); }

  // ---- Arduino File API ----
  size_t read(uint8_t* buf, size_t len) {
    if (!_impl || !_impl->fp) return 0;
    return fread(buf, 1, len, _impl->fp);
  }
  bool seek(uint32_t pos) {
    if (!_impl || !_impl->fp) return false;
    return fseek(_impl->fp, (long)pos, SEEK_SET) == 0;
  }
  size_t position() {
    if (!_impl || !_impl->fp) return 0;
    long p = ftell(_impl->fp);
    return p < 0 ? 0 : (size_t)p;
  }
  size_t size() {
    if (!_impl || !_impl->fp) return 0;
    long cur = ftell(_impl->fp);
    fseek(_impl->fp, 0, SEEK_END);
    long end = ftell(_impl->fp);
    if (cur >= 0) fseek(_impl->fp, cur, SEEK_SET);
    return end < 0 ? 0 : (size_t)end;
  }
  void close() {
    if (_impl) {
      if (_impl->fp) { fclose(_impl->fp); _impl->fp = nullptr; }
      _impl.reset();
    }
  }
  const char* name() const { return _impl ? _impl->name.c_str() : ""; }

  explicit operator bool() const { return _impl && _impl->fp != nullptr; }
};

// Wraps a VFS mount root (e.g. "/spiffs"). Paths handed in are absolute in the
// mount's namespace ("/new_prefs", "/bl/ab12") and are joined onto the root.
class FS {
  std::string _root;    // mount base path, e.g. "/spiffs"
  std::string _label;   // partition label for info()/format(), e.g. "spiffs"
public:
  FS() = default;
  FS(const char* root, const char* label = "")
    : _root(root ? root : ""), _label(label ? label : "") {}

  // mode: Arduino strings ("r","w","a","r+","w+","a+"); create is accepted for
  // Arduino-esp32 signature parity but POSIX "w"/"a" already create the file.
  File open(const char* path, const char* mode = "r", bool create = false);
  bool exists(const char* path);
  bool remove(const char* path);
  bool mkdir(const char* path);
  bool rmdir(const char* path);
  bool rename(const char* from, const char* to);
  bool format();
  size_t totalBytes();
  size_t usedBytes();

  const char* mountpoint() const { return _root.c_str(); }
};

// Internal-flash SPIFFS mount ("/spiffs", label "spiffs").
extern FS InternalFS;
extern FS SPIFFS;   // Arduino-esp32 spelling; same mount

} // namespace fs

// Arduino global alias.
typedef fs::File File;

// Mount (and format-if-needed) the "spiffs" partition at "/spiffs".
bool fs_mount_spiffs();
