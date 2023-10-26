#pragma once

#include "Config.h"
#include <string>
#include <vector>

/// TODO Rename this file to `Note` when finished

namespace RhythmTranscriber
{
    /// @brief Simplified note struct
    struct BaseNote
    {
        float timestamp = 0.f;
        float duration = 0.f;
    };
    class UniformNoteString
    {
    public:
        /// @brief Address of the first note of the string, from an array of notes.
        BaseNote *notes = nullptr;

        /// @brief Duration of the note string, usually the time between the first and last note of
        /// the string (including the last note's duration).
        float duration = 0.f;

        /// @brief Number of consecutive notes in the note string.
        unsigned int length = 0;

        class Interpretation
        {
        public:
            struct BeatRatio
            {
                float antecedent = 0.f;
                float consequent = 0.f;
                float quotient = 0.f;
            };

            /// @brief How much of a beat the interpretation represents
            BeatRatio ratio = BeatRatio{0.f, 0.f, 0.f};

            /// @brief How much of a beat each note represents
            BeatRatio noteRatio = BeatRatio{0.f, 0.f, 0.f};

            /// @brief Beats-per-minute of the interpretation
            float bpm = 0.f;

            float score = 0.f;
            float beatRatioScore = 0.f;
            float noteRatioScore = 0.f;

            float get_beat_ratio_score();
            float get_note_ratio_score();
        };

        std::vector<Interpretation> interpretations;

        float sd = 0.f;

        float get_sd();

        float get_score();

        void calc_duration();

        /// @brief Returns `true` if the note is considered to be uniform with this note string,
        /// `false` otherwise.
        /// @param note
        /// @return
        inline bool is_uniform(const BaseNote &note)
        {
            auto tailDuration = (notes + length - 1)->duration;
            return (tailDuration > note.duration ? (tailDuration / note.duration)
                                                 : (note.duration / tailDuration)) <
                   RhythmTranscriber::uniformRatioThreshold;
        }
        inline bool is_uniform(float duration)
        {
            auto tailDuration = (notes + length - 1)->duration;
            return (tailDuration > duration
                        ? (tailDuration / duration)
                        : (duration / tailDuration)) < RhythmTranscriber::uniformRatioThreshold;
        }

        /// @brief
        /// @note Calling this with a note string of length 1 will yield the same string for both
        /// elements of the pair.
        /// @return
        std::pair<UniformNoteString, UniformNoteString> split();

        inline std::pair<UniformNoteString, UniformNoteString> splitAt(unsigned int index)
        {
            return std::pair<UniformNoteString, UniformNoteString>(
                UniformNoteString{notes, (notes + index)->timestamp - notes->timestamp, index},
                UniformNoteString{notes + index,
                                  (notes + length - 1)->timestamp - (notes + index)->timestamp,
                                  length - index});
        }

        void create_interpretations();

        std::string str();

    private:
        float get_partial_score(unsigned int offset, unsigned int len);

        /* private:
            struct ScoreData
            {
                float score = -1.f;
                float sd = -1.f;
            };
            std::vector<float> scoreCache; */
    };
}