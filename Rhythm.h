#pragma once

#include <string>

namespace RhythmTranscriber
{
    struct BaseRhythm
    {
        unsigned char notes;
        unsigned char beats;
        unsigned int complexity;            // approx. notes * beats
        bool operator<(BaseRhythm &rhythm); // TODO Maybe remove
        bool operator>(BaseRhythm &rhythm); // TODO Maybe remove
    };
    struct NoteRhythm : BaseRhythm
    {
        float bpm;
        NoteRhythm(unsigned char notes, unsigned char beats, unsigned int complexity, float bpm);
        NoteRhythm();

        /// @brief Gets duration from BPM
        /// @return
        inline float get_duration() { return (60 * beats) / (bpm * notes); };
        /// @brief Sets BPM from duration
        /// @param duration
        inline void set_duration(float duration) { bpm = (60 * beats) / (duration * notes); };

        std::string str(bool compact = true);
    };
}