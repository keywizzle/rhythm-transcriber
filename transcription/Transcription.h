#pragma once

#include <string>
#include <vector>

/// Note: These data structures are mostly intended for user-created transcriptions.

namespace RhythmTranscriber
{
    namespace Transcription
    {
        /// @brief The hand the note should be played with.
        enum NoteHand
        {
            HAND_NONE = 0,
            HAND_RIGHT,
            HAND_LEFT,
            HAND_BOTH,
        };

        /// @brief The articulation/stick height of the note.
        enum NoteArticulation
        {
            ARTICULATION_NONE = 0,
            ARTICULATION_TAP,
            ARTICULATION_TENUTO,
            ARTICULATION_ACCENT,
        };

        /// @brief Represents data for an individual note's rhythm.
        struct NoteRhythm
        {
            /// `notes` and `beats` are unsigned shorts, giving them a max value of 65,535. This
            /// means the theoretical longest time a note could last would be 65,535 beats. At
            /// 120 BPM, this would be equivalent to roughly 9 hours, so it shouldn't be an
            /// issue.

            /// @brief Numerator of the rhythm's ratio.
            unsigned short notes = 0;

            /// @brief Denominator of the rhythm's ratio.
            unsigned short beats = 0;

            /// @brief Beats per minute of the rhythm.
            float bpm = 0.f;
        };

        /// @brief The drum in which the note is played. If the transcription is for the snare,
        /// every note will likely be `DRUM_SNARE`. For tenors, it will change depending on the
        /// drum.
        enum NoteDrum
        {
            DRUM_NONE = 0,
            DRUM_SNARE,
            DRUM_TENOR_1,
            DRUM_TENOR_2,
            DRUM_TENOR_3,
            DRUM_TENOR_4,
            DRUM_TENOR_SPOCK_1,
            DRUM_TENOR_SPOCK_2,
            /// TODO: Maybe add basses here
        };

        /// @brief The specific placement of the stick on the drum, including rimshots,
        /// rimclicks, etc.
        enum NotePlacement
        {
            PLACEMENT_NONE = 0,
            PLACEMENT_HEAD,
            PLACEMENT_EDGE,
            PLACEMENT_GUTS,
            PLACEMENT_SHELL,
            PLACEMENT_RIM_CLICK,
            PLACEMENT_RIM_SHOT,
            PLACEMENT_RIM_PING,
            PLACEMENT_RIM_GOCK,
        };

        /// @brief A data structure for storing data specific to a note at a timestamp in a
        /// transcription.
        struct NoteElement
        {
            /// @brief The timestamp of the `NoteElement`. This acts as a key, so for example
            /// you can have multiple `NoteElement`s with the same timestamp to represent one
            /// hand hitting the rim and the other hand hitting the shell.
            /// @note
            /// `NoteElement`s that are flams/grace notes should have the same timestamp as the
            /// main note it belongs to.
            float timestamp;

            /// @brief The duration of the note, in seconds. This is equal to the difference
            /// between the next note's timestamp and this note's timestamp.
            float duration = 0;

            NoteHand hand = NoteHand::HAND_NONE;

            NoteArticulation articulation = NoteArticulation::ARTICULATION_NONE;

            NotePlacement placement = NotePlacement::PLACEMENT_NONE;

            NoteRhythm rhythm;

            /// @brief `true` if the `NoteElement` is a flam/grace note.
            bool flam = false;

            /// @brief `true` if the note is a diddle. This will ultimately break the note into
            /// two notes with half the duration, f.e. an eighth note `NoteElement` with diddle
            /// as `true` represents two sixteenth notes. This will likely only be useful when
            /// displaying the transcription.
            bool diddle = false;

            /// @brief `true` if the note is buzzed.
            bool buzz = false;
        };

        struct Beat
        {
            /// @brief Timestamp of the start of the beat.
            float timestamp;

            /// @brief Duration of the beat (TODO: Isn't this the same as `division.duration`?).
            float duration = 0.f;

            // bpm...?
        };

        class Transcription
        {
        public:
            std::string name;

            std::vector<NoteElement> notes;

            std::vector<Beat> beats;

            /// @brief Checks if the transcription is valid.
            /// @return `true` if the transcription is valid.
            bool check();

            /// @brief Creates a transcription from a JSON string.
            /// @param json JSON string
            /// @return
            /// @note
            /// This method is fastest when the JSON string doesn't contain whitespace, aka
            /// non-pretty.
            static Transcription fromJSON(std::string json);

            /// @brief Creates a JSON string representing this transcription.
            /// @param compressed (optional) `true` by default, specifies if empty/default fields
            /// should be omitted from the output string.
            /// @return
            std::string toJSON(bool compressed = true);
        };
    }
}