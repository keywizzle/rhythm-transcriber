#pragma once

#include <vector>

#include "Config.h"
#include "Note.h"
#include "transcription/Transcription.h"

namespace RhythmTranscriber
{
    class Transcriber
    {
    public:
        std::vector<BaseNote> notes;

        RhythmTranscriber::Transcription::Transcription transcription;

        void transcribe(float *timestamps, unsigned int length);

    private:
        void create_notes(float *timestamps, unsigned int length);
    };
}
