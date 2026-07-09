/*
 * SdFat.h — P4/IDF stand-in for the Arduino SdFat library.
 *
 * SD on the T-Display-P4 is an SDMMC peripheral and is DEFERRED (a later
 * milestone). SdCard.h (`#include <SdFat.h>`) and the un-gated crash-report path
 * in UITask.cpp reference the SdFat surface (SdFs / FsFile / O_* flags), so this
 * header exposes just enough of that API — as inert stubs — for the shared UI to
 * COMPILE + LINK on P4. Every operation is a no-op that reports failure, and the
 * real `SdSvc::ready()/ensureMounted()` (p4_ui_stubs.cpp) always returns false, so
 * none of this is ever exercised at runtime.
 *
 * Resolved (via the companion component include path) only on the P4 build; the S3
 * builds pull the genuine <SdFat.h>. Guarded on P4_IDF_PLATFORM defensively.
 */
#pragma once

#if defined(P4_IDF_PLATFORM)

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>     // O_RDONLY / O_WRONLY / O_CREAT / O_TRUNC (POSIX)

// A no-op file handle. Every accessor reports "empty / failed".
class FsFile {
public:
  explicit operator bool() const { return false; }
  size_t write(const uint8_t* /*buf*/, size_t /*len*/) { return 0; }
  size_t write(uint8_t /*b*/) { return 0; }
  size_t read(uint8_t* /*buf*/, size_t /*len*/) { return 0; }
  int    fgets(char* /*out*/, int /*n*/) { return 0; }
  bool   seek(uint32_t /*pos*/) { return false; }
  size_t position() { return 0; }
  size_t size() { return 0; }
  int    available() { return 0; }
  bool   isDir() { return false; }
  void   rewind() {}
  bool   openNext(FsFile* /*dir*/, int /*mode*/ = O_RDONLY) { return false; }
  size_t getName(char* out, size_t cap) { if (out && cap) out[0] = 0; return 0; }
  void   close() {}
  void   flush() {}
};

// A no-op filesystem. Mount never succeeds (see SdSvc::ready() in p4_ui_stubs).
class SdFs {
public:
  bool   exists(const char* /*path*/) { return false; }
  bool   mkdir(const char* /*path*/) { return false; }
  bool   remove(const char* /*path*/) { return false; }
  FsFile open(const char* /*path*/, int /*flags*/ = O_RDONLY) { return FsFile(); }
};

#endif  // P4_IDF_PLATFORM
