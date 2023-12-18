#pragma once

#include "../Ratio.h"
#include "../transcription/Transcription.h"
#include <iostream>
#include <math.h>
#include <span>

namespace RhythmTranscriber
{
    class GuidoScore
    {
    public:
        struct ScoreFormat
        {
            std::string defaultNoteName = "a";
        };

        class Beat
        {
        public:
            std::span<Transcription::NoteElement> notes;

            Ratio offset = Ratio{0, 0};

            Ratio division = Ratio{0, 0};

            std::span<Transcription::NoteElement>::iterator notesEnd;

            bool tail_is_trimmed = false;

            bool offset_is_rest = false;

            inline bool is_uniform() { return isUniform; }

            Beat();

            Beat(std::span<Transcription::NoteElement> notes);

            Beat(Ratio offset, std::span<Transcription::NoteElement> notes);

            Beat create_next();

            std::string get_str();

            std::string get_gmn_str();

            inline bool is_tuplet()
            {
                auto divisionLog = std::log2(division.consequent);
                return std::floor(divisionLog) != divisionLog;
            }
            inline bool has_tail() { return division.antecedent > division.consequent; }
            inline Ratio get_tail()
            {
                return Ratio{division.antecedent - division.consequent, division.consequent};
            }

        private:
            bool isUniform = true;

            void add_notes(std::span<Transcription::NoteElement> notes);
        };

        ScoreFormat format = ScoreFormat{};

        std::vector<RhythmTranscriber::GuidoScore::Beat>::iterator currentBeat;

        std::vector<Beat> beats;

        GuidoScore(Transcription::Transcription transcription);

        std::string get_str();

    private:
        Transcription::Transcription transcription;

        std::string scoreStr;

        void add_meter(unsigned int numerator, unsigned int denominator);
        void add_meter(char numerator, char denominator);

        void add_clef();

        void add_note(unsigned int numerator, unsigned int denominator);

        void add_note(Transcription::NoteElement note);

        void add_note_value(Transcription::NoteElement note);

        void add_rest(Transcription::NoteElement note);

        /// @brief Adds current beat
        void add_beat();

        void fix_beat_format();
    };
}