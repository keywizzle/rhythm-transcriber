#pragma once

#include <vector>

#include "Config.h"
#include "NoteString.h"
#include "transcription/Transcription.h"

namespace RhythmTranscriber
{
    class Transcriber
    {
    public:
        std::vector<BaseNote> notes;

        std::vector<UniformNoteString> noteStrs;

        RhythmTranscriber::Transcription::Transcription transcription;

        void transcribe(float *timestamps, unsigned int length);

    private:
        void create_notes(float *timestamps, unsigned int length);

        void create_note_strs();

        void score_interpretations();

        void try_bpm(float bpm);
    };
}
