#pragma once
#include <stdint.h>

// Shared, output-agnostic RTTTL reader. Single source of truth for RTTTL parsing
// and note-frequency math (the % 12 semitone fix and the octave-5 scientific-pitch
// reference) so the piezo (tone()) and I2S (PCM synth) backends stay in tune with
// each other and Paul's pitch fixes live in exactly one place.
//
// Usage: begin(str) copies + parses the header (d=/o=/b=), then nextNote() pulls
// notes one at a time. Re-init-able: call begin() again to rewind (the I2S backend
// parses twice — once to count samples, once to render).
class RtttlReader {
public:
  // Copy + parse the header. Returns false if not a valid RTTTL string.
  bool begin(const char* rtttl);
  // Pull the next note. freqHz = 0 for a rest; durSec is the note length in seconds
  // (already includes the dotted-note 1.5x). Returns false at end of song.
  bool nextNote(float& freqHz, float& durSec);

private:
  char        _buf[512];
  const char* _note_ptr = nullptr;
  int         _def_dur  = 4;
  int         _def_oct  = 5;
  int         _bpm      = 120;
};

// semitone 0=C .. 11=B; octave per RTTTL (5 = scientific C5 ~523 Hz).
float rtttlNoteFreq(int semitone, int octave);
