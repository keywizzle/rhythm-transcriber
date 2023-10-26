#pragma once

#include "Beat.h"

namespace RhythmTranscriber
{
    extern unsigned int divisionDepth;

    extern unsigned int beatDivisions[];

    const unsigned int branchBufferSize = 8;
    /* const unsigned int branchBufferSize = 10; */

    /* extern float beatDivisionScores[];

    extern unsigned int beatDivisionsSize; */

    class BeatData
    {
    public:
        /// @brief Pointer to first note.
        BaseNote *notes = nullptr;

        unsigned int length = 0;

        /// TODO: Change these to a single short value and use bit flags to get offbeat data.

        bool startsOffbeat = false;
        bool endsOffbeat = false;

        std::string str();

        inline float get_duration() { return (notes + length)->timestamp - notes->timestamp; }
    };

    /// This class is supposed to be used as a singleton.

    class BeatBranch
    {
    public:
        /// TODO: Need to add SSO-like support for possibility that user sets `maxDepth` to a high
        /// value and to allocate on the heap in that case.

        BeatData dataBuffer[branchBufferSize];

        Beat beatBuffer[branchBufferSize];

        unsigned int length = 0;

        /// @brief Represents how much the BPM changes over the beats. The less the BPM changes, the
        /// higher the score.
        float bpmDeltaScore = 0.f;

        /// @brief Average distance to a reference BPM. The lower the distance, the higher the
        /// score.
        float bpmDistScore = 0.f;

        /// @brief Average score of the beats in `beatBuffer`.
        float beatAvgScore = 0.f;

        void create_beats();

        float calc_score(float referenceBeatDuration);

        std::string data_buffer_str();
        std::string beat_buffer_str();

        std::string str();

    private:
        void interpret_division_string(unsigned int beatIndex, unsigned int beatLength,
                                       unsigned int noteLength);

        void expand_beat(unsigned int beatIndex, unsigned int beatLength);
    };
}