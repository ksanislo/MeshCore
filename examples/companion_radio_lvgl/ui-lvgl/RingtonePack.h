#pragma once
#include <stddef.h>

// On-device ringtone pack downloader. Fetches ringtones/pack.txt from the repo
// via raw.githubusercontent.com and writes each RTTTL string to its own
// /ringtones/<Name>.rtttl file on the SD card so they appear in the ringtone picker.
//
// Mirrors the EmojiPack pattern: runs on a core-1 task, sets busy()/status()
// for the UI to poll, and can be cancelled between files.
namespace RingtonePack {
  void start();                         // spawn the download task (no-op if already running)
  void cancel();                        // request abort between files
  bool busy();
  void status(char* out, size_t cap);   // human-readable status for the Sound settings pane
}
