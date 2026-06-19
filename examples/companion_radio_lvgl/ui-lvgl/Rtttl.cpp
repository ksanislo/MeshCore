#include "Rtttl.h"
#include <string.h>
#include <ctype.h>

// Note frequency table (octave 4 reference, semitone 0=C … 11=B).
static const float NOTE_FREQ_OCT4[12] = {
    261.626f, 277.183f, 293.665f, 311.127f, 329.628f, 349.228f,
    369.994f, 391.995f, 415.305f, 440.000f, 466.164f, 493.883f
};

float rtttlNoteFreq(int semitone, int octave) {
    float f = NOTE_FREQ_OCT4[semitone % 12];
    // RTTTL octave 5 = C at 523 Hz (scientific pitch C5). Our table is C4 (261 Hz),
    // so shift by (octave - 5) not (octave - 4) — otherwise all notes play one octave high.
    int d = octave - 5;
    if (d > 0)      f *= (float)(1 << d);
    else if (d < 0) f /= (float)(1 << (-d));
    return f;
}

static int letterSemitone(char c) {
    switch (tolower((unsigned char)c)) {
        case 'c': return 0; case 'd': return 2; case 'e': return 4;
        case 'f': return 5; case 'g': return 7; case 'a': return 9;
        case 'b': return 11;
        default:  return -1;
    }
}

bool RtttlReader::begin(const char* rtttl) {
    if (!rtttl || !*rtttl) return false;
    strncpy(_buf, rtttl, sizeof(_buf) - 1);
    _buf[sizeof(_buf) - 1] = '\0';

    const char* p = strchr(_buf, ':');
    if (!p) return false;
    p++;

    _def_dur = 4; _def_oct = 5; _bpm = 120;

    while (*p && *p != ':') {
        while (*p == ' ' || *p == ',') p++;
        if (!*p || *p == ':') break;
        char key = (char)tolower((unsigned char)*p++);
        if (*p == '=') p++;
        int val = 0;
        while (*p >= '0' && *p <= '9') { val = val*10 + (*p - '0'); p++; }
        switch (key) {
            case 'd': if (val) _def_dur = val; break;
            case 'o': if (val) _def_oct = val; break;
            case 'b': if (val) _bpm     = val; break;
        }
    }
    if (*p == ':') p++;
    _note_ptr = p;
    return *p != '\0';
}

bool RtttlReader::nextNote(float& freqHz, float& durSec) {
    if (!_note_ptr || !*_note_ptr) return false;

    const char* p = _note_ptr;
    while (*p == ',' || *p == ' ' || *p == '\t') p++;
    if (!*p) return false;

    int dur = _def_dur;
    if (*p >= '1' && *p <= '9') {
        dur = 0;
        while (*p >= '0' && *p <= '9') { dur = dur*10 + (*p - '0'); p++; }
        if (!dur) dur = _def_dur;
    }

    int semi  = letterSemitone(*p);
    bool rest = (*p == 'p' || *p == 'P');
    p++;

    if (*p == '#') { if (!rest && semi >= 0) semi++; p++; }

    bool dot = false;
    if (*p == '.') { dot = true; p++; }

    int oct = _def_oct;
    if (*p >= '4' && *p <= '7') { oct = *p - '0'; p++; }
    if (*p == '.') { dot = true; p++; }

    _note_ptr = p;

    float secs = (4.0f / (float)dur) * (60.0f / (float)_bpm);
    if (dot) secs *= 1.5f;
    durSec = secs;

    freqHz = (rest || semi < 0) ? 0.0f : rtttlNoteFreq(semi, oct);
    return true;
}
