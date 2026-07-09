/*
 * SPIFFS.h — P4/IDF stand-in for Arduino-esp32's <SPIFFS.h>.
 *
 * UITask.cpp includes <SPIFFS.h> for the internal-flash crash-report fallback
 * (`SPIFFS.open("/last_crash.txt", FILE_WRITE)`). On pure ESP-IDF there is no such
 * header; the fs_shim (components/fs_shim/FS.h) already provides an equivalent
 * fs::FS mounted on the "spiffs" partition as `fs::SPIFFS`. This header just
 * surfaces the Arduino spellings (`SPIFFS`, `FILE_WRITE`, `File`) that UITask uses,
 * so the shared UI compiles unchanged on P4.
 *
 * Resolved (via the companion component include path) only on the P4 build.
 */
#pragma once

#if defined(P4_IDF_PLATFORM)

#include <FS.h>   // fs_shim: fs::FS, fs::File, global `File` alias, fs::SPIFFS

// Arduino global spelling of the internal-flash mount.
using fs::SPIFFS;

// Arduino file-open mode strings (fs::FS::open takes an fopen-style mode string).
#ifndef FILE_READ
  #define FILE_READ  "r"
#endif
#ifndef FILE_WRITE
  #define FILE_WRITE "w"
#endif
#ifndef FILE_APPEND
  #define FILE_APPEND "a"
#endif

#endif  // P4_IDF_PLATFORM
