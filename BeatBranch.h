#pragma once

#include "Beat.h"

namespace RhythmTranscriber
{
    extern unsigned int divisionDepth;

    extern unsigned int beatDivisions[];

    /* const unsigned int branchBufferSize = 8; */
    const unsigned int branchBufferSize = 15;

    class BeatData
    {
    public:
        /// @brief Pointer to first note.
        BaseNote *notes = nullptr;

        unsigned int length = 0;

        /// TODO: Change these to a single short value and use bit flags to get offbeat data.

        bool startsOffbeat = false;
        bool endsOffbeat = false;

        bool needsUpdate = true;

        inline float get_duration() { return (notes + length)->timestamp - notes->timestamp; }

        std::string str();
    };

    class BeatBranch
    {
    public:
        /// TODO: Need to add SSO-like support for possibility that user sets `maxDepth` to a high
        /// value and to allocate on the heap in that case.

        BeatData dataBuffer[branchBufferSize];

        Beat beatBuffer[branchBufferSize];

        unsigned int length = 0;

        /// @brief This is only needed in a very particular circumstance when dealing with notes
        /// that last longer than a quarter note.
        float expectedBeatDuration = 0.f;

        /// Based on the data in `dataBuffer`, creates beats to match it with the best
        /// matching rhythms and stores them in `beatBuffer`.
        /// Returns `true` if the beats were successfully created, `false` otherwise. Different
        /// things that can cause beats to not be created:
        /// - Beat was unable to get valid note divisions.
        /// - Every beat is an offbeat in `dataBuffer`.
        bool create_beats();

        /// @brief Checks to see if the beat at `beatIndex` correctly reflects what is defined in
        /// `dataBuffer`.
        /// @param beatIndex
        /// @return
        inline bool beat_matches_data_at(unsigned int beatIndex)
        {
            return (dataBuffer[beatIndex].length == beatBuffer[beatIndex].notesLen &&
                    dataBuffer[beatIndex].startsOffbeat ==
                        (bool)beatBuffer[beatIndex].offset.antecedent &&
                    dataBuffer[beatIndex].endsOffbeat ==
                        (beatBuffer[beatIndex].division.antecedent >
                         beatBuffer[beatIndex].division.consequent));
        }

        std::string data_buffer_str();
        std::string beat_buffer_str();

        std::string str();

    private:
        /// @brief Creates beat string at an index based on the data in `dataBuffer`.
        /// @param beatIndex Index of the beat/beat data.
        /// @param beatLength Effectively how many beats to treat as a single beat.
        /// @param noteLength Total number of notes in the beat(s).
        /// @return `true` if `beatBuffer matchers `dataBuffer`, `false` otherwise.
        bool create_beats_at(unsigned int beatIndex, unsigned int beatLength,
                             unsigned int noteLength);

        /// @brief Creates beat string at an index based on the data in `dataBuffer`. This is used
        /// for beats that don't end on a downbeat.
        /// @param beatIndex Index of the beat/beat data.
        /// @param beatLength Effectively how many beats to treat as a single beat.
        /// @param noteLength Total number of notes in the beat(s).
        /// @param effectiveBeatDuration Duration of the beat(s).
        /// @return `true` if `beatBuffer matchers `dataBuffer`, `false` otherwise.
        bool create_beats_at(unsigned int beatIndex, unsigned int beatLength,
                             unsigned int noteLength, float effectiveBeatDuration);

        void expand_beat(unsigned int beatIndex, unsigned int beatLength);

        inline bool beats_need_update(unsigned int beatIndex, unsigned int beatLength)
        {
            for (unsigned int i = beatIndex; i < beatIndex + beatLength; i++)
            {
                if (dataBuffer[i].needsUpdate)
                {
                    return true;
                }
            }

            return false;
        }

        inline void set_updated(unsigned int beatIndex, unsigned int beatLength)
        {
            for (unsigned int i = beatIndex; i < beatIndex + beatLength; i++)
            {
                dataBuffer[i].needsUpdate = false;
            }
        }
    };
}