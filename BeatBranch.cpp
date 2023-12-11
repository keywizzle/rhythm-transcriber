#include "BeatBranch.h"

#include <math.h>

namespace RhythmTranscriber
{
    unsigned int divisionDepth = 14;

    unsigned int beatDivisions[] = {1,  2, 4,  3,  6,  12, 8,  5,  10, 9,  18, 16,
                                    20, 7, 14, 15, 21, 11, 22, 13, 17, 19, 23};

    /* float beatDivisionScores[] = {
        1,          1,          0.75,       2.0 / 3.0,  2.0 / 3.0,  0.5f,       0.5f,
        0.4f,       0.4f,       1.0 / 3.0,  5.0 / 16.0, 0.3,        2.0 / 7.0,  4.0 / 15.0,
        4.0 / 21.0, 2.0 / 11.0, 2.0 / 11.0, 2.0 / 13.0, 2.0 / 17.0, 2.0 / 19.0, 2.0 / 23.0};

    unsigned int beatDivisionsSize = 21; */

    bool BeatBranch::create_beats()
    {
        unsigned int divisionBeatLen = 0;
        unsigned int divisionNoteLen = 0;

        unsigned int beatIndex = 0;

        float recentDownbeatTime = dataBuffer[0].notes[0].timestamp;
        unsigned int recentDownbeatIndex = 0;

        bool shouldUpdate = false;

        for (unsigned int i = 0; i < length; i++)
        {
            /// @todo See if it's faster to avoid copying to local variable.
            auto data = dataBuffer[i];

            if (data.needsUpdate)
            {
                shouldUpdate = true;
            }

            if (data.length > 0 && data.notes[data.length - 1].duration > expectedBeatDuration * 2)
            {
                /// We may need to add more beats depending on where the long note falls into the
                /// beat.

                /// In testing, if the combined note duration is even slightly less than
                /// `expectedBeatDuration`, it lowers the beat count by 1, which will give incorrect
                /// results. Adding 0.25 shifts this threshold over to give a bit more room for
                /// error.

                divisionBeatLen +=
                    (((data.notes[data.length - 1].timestamp - data.notes[0].timestamp) +
                      data.notes[data.length - 1].duration) /
                     expectedBeatDuration) +
                    0.25f;
            }
            else
            {
                divisionBeatLen++;
            }

            divisionNoteLen += data.length;

            if (!data.endsOffbeat)
            {
                if (shouldUpdate)
                {
                    if (!create_beats_at(beatIndex, divisionBeatLen, divisionNoteLen))
                    {
                        return false;
                    }

                    set_updated(beatIndex, i - recentDownbeatIndex + 1);
                    /* set_updated(beatIndex, divisionBeatLen); */

                    shouldUpdate = false;
                }

                divisionBeatLen = 0;
                divisionNoteLen = 0;

                beatIndex = i + 1;

                /// Avoid going out of bounds on the last index while also avoiding branching. It's
                /// gooberish, I know. Maybe there's a better way to do it.
                recentDownbeatTime = dataBuffer[i + 1 - (i == length - 1)].notes[0].timestamp;
                recentDownbeatIndex = i + 1;
            }
        }

        if (divisionBeatLen != 0)
        {
            /// Branch ends on `divisionBeatLen` number of offbeats. Here, we try to figure out what
            /// they are without using a downbeat note.

            if (divisionBeatLen >= length)
            {
                /// No downbeat in this branch.
                return false;
            }

            /// Local duration must be long enough to include all notes in `dataBuffer` at the
            /// index. If it's too short, it will essentially round to the downbeat of the last
            /// note.
            float localDuration =
                std::max(dataBuffer[length - divisionBeatLen].notes[divisionNoteLen - 1].timestamp -
                             dataBuffer[length - 1].notes->timestamp,
                         beatBuffer[length - divisionBeatLen - 1].get_duration());

            return create_beats_at(length - divisionBeatLen, divisionBeatLen, divisionNoteLen,
                                   dataBuffer[length - 1].notes->timestamp + localDuration -
                                       dataBuffer[length - divisionBeatLen].notes->timestamp);
        }

        return true;
    }

    bool BeatBranch::create_beats_at(unsigned int beatIndex, unsigned int beatLength,
                                     unsigned int noteLength)
    {
        Beat beat = Beat(dataBuffer[beatIndex].notes, noteLength);

        float bestScore = 0.f;
        float beatScore;

        for (unsigned int i = 0; i <= divisionDepth; i++)
        {
            /// Multiplying the division by `beatLength` will allow us to use a single beat instance
            /// to score multiple beats (that don't start/end on beat) as one. This has it's
            /// issues, but for right now it seems to work alright doing it this way instead of
            /// trying to separate them and deal with offbeat beats individually.

            /// Any division that would normally be possible for a single beat will be 1:1 to
            /// multiple beats, just as multiples. For example, if 9 is a possible division
            /// normally and `beatLength` is 2, 9 will become 18, and 9 will not be tested.

            auto division = beatDivisions[i] * beatLength;

            if (division < noteLength)
            {
                /// The beat will not be able to represent a division lower than `noteLength`.
                /// Example: representing 5 notes in a beat as sixteenth notes isn't possible.
                continue;
            }

            if (!beat.set_note_ratios(division))
            {
                /// Weren't able to correctly set note ratios based off of `division`.
                continue;
            }

            /// Since we may be using the beat as a combination of multiple beats, to get accurate
            /// scores we need to modify the division to be what it *would* be when the beats are
            /// split out, which will cause note ratios to add up to be `beatLength` beats. This is
            /// essentially reversing the multiplication of the division done earlier.
            beat.division.consequent /= beatLength;

            /// We should really be scoring the expanded beat instead, but this seems to work for
            /// the most part.
            beatScore = beat.calc_score();

            if (beatScore > bestScore)
            {
                /// Add tiny amount to prevent a beat with an equal score overriding this one due to
                /// floating point comparison issues.
                bestScore = beatScore + 0.000001f;

                /// Copy to buffer at startIndex. If our beat is actually a "multi-beat", we will
                /// expand it to fill the buffer accordingly a little later.
                beatBuffer[beatIndex] = beat;
            }
        }

        if (bestScore == 0.f)
        {
            /// After trying every division, we still couldn't create the beat.
            return false;
        }

        if (beatLength > 1)
        {
            expand_beat(beatIndex, beatLength);
        }

        return true;
    }

    bool BeatBranch::create_beats_at(unsigned int beatIndex, unsigned int beatLength,
                                     unsigned int noteLength, float effectiveBeatDuration)
    {
        /// @todo Combine into a single `create_beats_at` method.

        Beat beat = Beat(dataBuffer[beatIndex].notes, noteLength, effectiveBeatDuration);

        float bestScore = 0.f;
        float beatScore;

        for (unsigned int i = 0; i <= divisionDepth; i++)
        {
            /// Multiplying the division by `beatLength` will allow us to use a single beat instance
            /// to score multiple beats (that don't start/end on beat) as one. This has it's
            /// issues, but for right now it seems to work alright doing it this way instead of
            /// trying to separate them and deal with offbeat beats individually.

            /// Any division that would normally be possible for a single beat will be 1:1 to
            /// multiple beats, just as multiples. For example, if 9 is a possible division
            /// normally and `beatLength` is 2, 9 will become 18, and 9 will not be tested.

            auto division = beatDivisions[i] * beatLength;

            if (division < noteLength)
            {
                /// The beat will not be able to represent a division lower than `noteLength`.
                /// Example: representing 5 notes in a beat as sixteenth notes isn't possible.
                continue;
            }

            if (!beat.set_offbeat_note_ratios(division))
            {
                /// Weren't able to correctly set note ratios based off of `division`.
                continue;
            }

            /// Since we may be using the beat as a combination of multiple beats, to get accurate
            /// scores we need to modify the division to be what it *would* be when the beats are
            /// split out, which will cause note ratios to add up to be `beatLength` beats. This is
            /// essentially reversing the multiplication of the division done earlier.
            beat.division.consequent /= beatLength;

            beatScore = beat.calc_score();

            if (beatScore > bestScore)
            {
                /// Add tiny amount to prevent a beat with an equal score overriding this one
                /// due to floating point comparison issues.
                bestScore = beatScore + 0.000001f;

                /// Copy to buffer at startIndex. If our beat is actually a "multi-beat", we
                /// will expand it to fill the buffer accordingly a little later.
                beatBuffer[beatIndex] = beat;
            }
        }

        if (bestScore == 0.f)
        {
            /// After trying every division, we still couldn't create the beat.
            return false;
        }

        if (beatLength > 1)
        {
            expand_beat(beatIndex, beatLength);
        }
        else
        {
            /// It's possible to have beatLength of 1 with an offbeat if the beat is the last one of
            /// the branch. We need to clean up the beat a little bit (which would normally be done
            /// by `expand_beat`) before returning.

            /// Fix beat end time.
            beatBuffer[beatIndex].calc_time();
        }

        return true;
    }

    void BeatBranch::expand_beat(unsigned int beatIndex, unsigned int beatLength)
    {
        /// @todo Optimize by having local variables keep track of division, notes pointer, notes
        /// length, etc. instead of keeping track of it on beatBuffer, or a beat instance at all.

        /// Copy multi-beat.
        auto multiBeat = beatBuffer[beatIndex];

        unsigned int consequent = multiBeat.division.consequent;

        beatBuffer[beatIndex] = Beat(multiBeat.notes);

        /// Set this now to avoid having to simplify later.
        beatBuffer[beatIndex].division.consequent = consequent;

        for (unsigned int i = 0; i < multiBeat.notesLen; i++)
        {
            /// The note ratios still have the consequent as beatLength * division, so fix it here
            /// before adding.
            beatBuffer[beatIndex].add_note(
                NoteRatio{multiBeat.noteRatios[i].antecedent, multiBeat.division.consequent});

            if (beatBuffer[beatIndex].division.antecedent >= consequent)
            {
                beatBuffer[beatIndex].calc_note_partials();
                beatBuffer[beatIndex].calc_score();

                beatIndex++;

                /// Create (initialize) next beat
                beatBuffer[beatIndex] = beatBuffer[beatIndex - 1].create_next();
            }
        }
    }

    std::string BeatData::str()
    {
        std::string str = "";

        str += '[';
        for (unsigned int i = 0; i < length; i++)
        {
            str += std::to_string(notes[i].timestamp);
            if (i < length - 1)
            {
                str += ", ";
            }
        }
        str += "] (" + std::to_string(length) + ") ";

        str += "startsOffbeat: " + std::to_string(startsOffbeat) +
               ", endsOffbeat: " + std::to_string(endsOffbeat);

        return str;

        /* str += (notes == nullptr
                    ? "nullptr"
                    : (std::to_string(notes->timestamp) + (startsOffbeat ? "(?)" : "") + "->" +
                       (length == 0 ? "nullptr"
                                    : std::to_string(notes[length - 1].timestamp) +
                                          (endsOffbeat ? "(?)" : "")))) +
               " (" + std::to_string(length) +
               "), startsOffbeat: " + std::to_string(startsOffbeat) +
               ", endsOffbeat: " + std::to_string(endsOffbeat);
        return str; */

        float startTime =
            startsOffbeat ? (((notes - 1)->timestamp + notes->timestamp) / 2) : notes->timestamp;
        float endTime = endsOffbeat
                            ? (((notes + length - 1)->timestamp + (notes + length)->timestamp) / 2)
                            : (notes + length)->timestamp;

        str += std::to_string(startTime) + "->" + std::to_string(endTime) + " (" +
               std::to_string(length) + "), startsOffbeat: " + std::to_string(startsOffbeat) +
               ", endsOffbeat: " + std::to_string(endsOffbeat);
        /* str += "notes: " + (notes == nullptr ? "nullptr" : std::to_string(startTime)) + "->" +
               (length == 0 ? "null" : std::to_string(endTime)) + " (" + std::to_string(length) +
               "), startsOffbeat: " + std::to_string(startsOffbeat) +
               ", endsOffbeat: " + std::to_string(endsOffbeat); */
        return str;
    }

    std::string BeatBranch::data_buffer_str()
    {
        std::string str = "";
        for (unsigned int i = 0; i < length; i++)
        {
            str += dataBuffer[i].str() + '\n';
        }
        return str;
    }
    std::string BeatBranch::beat_buffer_str()
    {
        std::string str = "";
        for (unsigned int i = 0; i < length; i++)
        {
            str += beatBuffer[i].str() + '\n';
        }
        return str;
    }

    std::string BeatBranch::str() { return data_buffer_str() + beat_buffer_str(); }
}