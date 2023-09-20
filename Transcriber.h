#pragma once

#include <vector>

#include "Config.h"
#include "Note.h"
#include "Rhythm.h"

namespace RhythmTranscriber
{
    class Transcriber
    {
    public:
        BaseRhythm baseRhythms[rhythmCount];

        std::vector<Note> notes;

        void init_rhythms();

        void create_notes(float *timestamps, unsigned int length);

        void label_neighbor_ratios();

        unsigned int get_note_string(unsigned int noteIndex, float bpm);

        UniformNoteString get_uniform_note_str_at(unsigned int index);

        /// @brief Does returning a local vector create a copy for whatever it's assigned to? Maybe
        /// this is okay
        /// @return
        std::vector<UniformNoteString> get_uniform_note_strs();

        void transcribe(float *timestamps, unsigned int length, float bpm);

    private:
        bool loadedRhythms = false;

        float initialBpm;
    };
}
