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

    void BeatBranch::create_beats()
    {
        /* for (unsigned int i = 0; i < length; i++)
        {
            std::cout << "data: " << dataBuffer[i].str() << '\n';
        } */

        unsigned int divisionBeatLen = 0;
        unsigned int divisionNoteLen = 0;

        for (unsigned int i = 0; i < length; i++)
        {
            auto data = dataBuffer[i];

            divisionNoteLen += data.length;
            divisionBeatLen++;

            /// TODO: Handle if last beat in branch does not end on a downbeat. Since we're limited
            /// by `maxDepth`, we can't use further notes to guess. The next downbeat is between the
            /// last note and the note after the last note.
            /// For now, we're omitting

            if (!data.endsOffbeat)
            {
                interpret_division_string(i + 1 - divisionBeatLen, divisionBeatLen,
                                          divisionNoteLen);
                divisionBeatLen = 0;
                divisionNoteLen = 0;
            }
        }

        /* for (unsigned int i = 0; i < length; i++)
        {
            std::cout << "beat: " << beatBuffer[i].str() << '\n';
        }
        std::cout << "****************************\n"; */
    }

    void BeatBranch::interpret_division_string(unsigned int beatIndex, unsigned int beatLength,
                                               unsigned int noteLength)
    {
        if (beatLength == 1 && dataBuffer[beatIndex].notes == beatBuffer[beatIndex].notes &&
            dataBuffer[beatIndex].length == beatBuffer[beatIndex].notesLen)
        {
            /// Nice and cheesy little optimization here to avoid extra work.
            return;
        }

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

            beat.set_note_ratios(division);

            /// Since we may be using the beat as a combination of multiple beats, to get accurate
            /// scores we need to modify the division to be what it *would* be when the beats are
            /// split out, which will cause note ratios to add up to be `beatLength` beats. This is
            /// essentially reversing the multiplication of the division done earlier.
            beat.division.consequent /= beatLength;

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

            /* std::cout << "score for division " << division << ": " << beatScore
                      << " (distScore: " << beat.distScore
                      << ", divisionScore: " << beat.divisionScore
                      << ", noteScore: " << beat.noteScore << ")" << '\n'; */
        }

        if (beatLength > 1)
        {
            expand_beat(beatIndex, beatLength);
        }
    }

    void BeatBranch::expand_beat(unsigned int beatIndex, unsigned int beatLength)
    {
        /// @todo Optimize by having local variables keep track of division, notes pointer, notes
        /// length, etc. instead of keeping track of it on beatBuffer, or a beat instance at all.

        /// Copy multi-beat.
        auto multiBeat = beatBuffer[beatIndex];

        unsigned int consequent = multiBeat.division.consequent;

        beatBuffer[beatIndex] = Beat(multiBeat.notes, 0);

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

                /// Create next beat
                beatBuffer[beatIndex] = beatBuffer[beatIndex - 1].create_next();
            }
        }
    }

    float BeatBranch::calc_score(float referenceBeatDuration)
    {
        bpmDeltaScore = 0.f;
        bpmDistScore = 0.f;

        float avgBeatScore = 0.f;

        /// TODO: Maybe have a local BPM score: how well each beat fits with the local BPM.
        /// This is similar to delta score, except look at further beats than only the neighboring
        /// one.

        /// Sometimes, a phrase of notes with a common division will choose a different
        /// division between beats if the notes slightly speed up or slow down. For example, 1/8
        /// notes switching to 1/7 notes the next beat. This wouldn't be a problem (in theory) if
        /// the max depth was really high, because the incorrect division will cause chaos later
        /// down the line and get a low score. For lower depths, we can use this score to fairly
        /// confidently catch this type of thing.
        /// Example: referenceBPM: ~132, beat of 1/8 notes at 118 BPM followed by beat of 1/7 notes
        /// at 137 BPM
        float beatNeighborRelationScore = 0.f;
        float prevBaseDuration = beatBuffer[0].duration / beatBuffer[0].division.antecedent;

        unsigned int beatDivScoreCount = beatBuffer[0].division.antecedent;

        for (unsigned int i = 0; i < length; i++)
        {
            if ((dataBuffer[i].length != beatBuffer[i].notesLen) ||
                (dataBuffer[i].startsOffbeat && beatBuffer[i].offset.antecedent == 0))
            {
                /// For now at least, if the note lengths or offsets don't match, then assume that
                /// the specifications in `dataBuffer` are not feasibly possible.
                return 0.f;
            }

            float beatDuration = beatBuffer[i].get_duration();

            bpmDistScore +=
                1 - (beatDuration < referenceBeatDuration ? beatDuration / referenceBeatDuration
                                                          : referenceBeatDuration / beatDuration);

            if (i < length - 1)
            {
                float nextDuration = beatBuffer[i + 1].get_duration();
                bpmDeltaScore = 1 - (beatDuration < nextDuration ? beatDuration / nextDuration
                                                                 : nextDuration / beatDuration);
            }

            avgBeatScore += beatBuffer[i].score;

            auto baseDuration = beatBuffer[i].duration / beatBuffer[i].division.antecedent;

            if (i > 0 &&
                beatBuffer[i].division.antecedent != beatBuffer[i - 1].division.antecedent &&
                std::abs(baseDuration - prevBaseDuration) < 0.05f)
            {
            }

            prevBaseDuration = baseDuration;
        }

        bpmDeltaScore = length == 1 ? 1.f : bpmDeltaScore / (length - 1);
        bpmDistScore = bpmDistScore / length;

        /// Scale
        /* bpmDeltaScore = 0.01f / (bpmDeltaScore * bpmDeltaScore + 0.01f); */
        bpmDeltaScore = 0.02f / (bpmDeltaScore * bpmDeltaScore + 0.02f);

        bpmDistScore = 0.0625f / (bpmDistScore * bpmDistScore + 0.0625f);

        avgBeatScore /= length;

        /* return 0.5 * bpmDeltaScore + 0.5 * bpmDistScore; */
        /* return score = 0.375 * bpmDeltaScore + 0.375 * bpmDistScore + 0.25 * avgBeatScore; */
        return score = 0.25f * bpmDeltaScore + 0.35f * bpmDistScore + 0.4f * avgBeatScore;
    }

    std::string BeatData::str()
    {
        std::string str = "";

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