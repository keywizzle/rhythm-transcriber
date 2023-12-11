#pragma once

#include "../Beat.h"
#include "../BeatAnalyzer.h"
#include "../BeatBranch.h"
#include "../transcription/TranscriptionFile.h"
#include "../utils.h"
#include <iostream>

std::string format_time(LONGLONG timeMicroseconds)
{
    std::string timeStr = "";

    if (timeMicroseconds < 1000)
    {
        timeStr += std::to_string(timeMicroseconds) + " microseconds";
    }
    else if (timeMicroseconds < 1000000)
    {
        timeStr += std::to_string((double)timeMicroseconds / 1000) + " milliseconds";
    }
    else
    {
        timeStr += std::to_string((double)timeMicroseconds / 1000000) + " seconds";
    }

    return timeStr;
}

namespace RhythmTranscriber
{
    namespace
    {
        class TestSource
        {
        public:
            std::string fileName;

            std::string prefix = "TestSource: ";

            Transcription::Transcription transcription;

            std::vector<BaseNote> notes;

            std::vector<BaseRatio> noteRatios;

            TestSource() {}

            TestSource(std::string fileName)
            {
                this->fileName = fileName;
                prefix += '\"' + fileName + '\"';

                Transcription::TranscriptionFile transcriptionFile =
                    Transcription::TranscriptionFile();
                transcription = transcriptionFile.read(".\\test data\\" + fileName + ".json");

                for (unsigned int i = 0; i < transcription.notes.size(); i++)
                {
                    auto note = transcription.notes[i];

                    if (note.flam)
                        continue;

                    notes.push_back(BaseNote{note.timestamp, transcription.notes[i + 1].timestamp -
                                                                 note.timestamp});
                    noteRatios.push_back(BaseRatio{note.rhythm.beats, note.rhythm.notes});
                }
            }

            bool test_beat_scores(unsigned int noteIndex, unsigned int beatLength)
            {
                std::string prefix = this->prefix + " test_beat_scores(" +
                                     std::to_string(noteIndex) + ", " + std::to_string(beatLength) +
                                     "): ";

                bool failStatus = false;
                std::string errorMsg = prefix + "Failed\n";

                BeatBranch expectedBranch = create_branch(noteIndex, beatLength, true);

                BeatBranch actualBranch = create_branch(noteIndex, beatLength);
                actualBranch.create_beats();

                unsigned int noteRatioIndex = noteIndex;

                for (unsigned int i = 0; i < expectedBranch.length; i++)
                {
                    auto expectedBeat = expectedBranch.beatBuffer[i];

                    auto actualBeat = actualBranch.beatBuffer[i];

                    std::string beatCmpErrorMsg = compare_beats(expectedBeat, actualBeat);

                    if (beatCmpErrorMsg != "")
                    {
                        failStatus = true;
                        errorMsg += beatCmpErrorMsg;
                    }
                }

                if (failStatus)
                {
                    std::cout << errorMsg << '\n';
                    /* std::cout << "- Expected data:\n";
                    for (unsigned int i = 0; i < expectedBranch.length; i++)
                    {
                        std::cout << "\t" << expectedBranch.dataBuffer[i].str() << '\n';
                    }
                    std::cout << "- Actual data:\n";
                    for (unsigned int i = 0; i < actualBranch.length; i++)
                    {
                        std::cout << "\t" << actualBranch.dataBuffer[i].str() << '\n';
                    } */
                    std::cout << "- Expected beats:\n";
                    for (unsigned int i = 0; i < expectedBranch.length; i++)
                    {
                        expectedBranch.beatBuffer[i].calc_note_partials();
                        expectedBranch.beatBuffer[i].calc_score();
                        std::cout << "\t" << expectedBranch.beatBuffer[i].str() << '\n';
                    }
                    std::cout << "- Actual beats:\n";
                    for (unsigned int i = 0; i < actualBranch.length; i++)
                    {
                        std::cout << "\t" << actualBranch.beatBuffer[i].str() << '\n';
                    }
                }
                else
                {
                    std::cout << prefix << "Passed\n";
                }
                return failStatus;
            }

            bool test_branch_scores(unsigned int noteIndex, unsigned int beatLength, float bpm)
            {
                std::string prefix = this->prefix + " test_branch_scores(" +
                                     std::to_string(noteIndex) + ", " + std::to_string(beatLength) +
                                     "): ";
                bool failStatus = false;
                std::string errorMsg = prefix + "Failed\n";

                BeatAnalyzer beatAnalyzer;

                beatAnalyzer.notes = &(notes[0]);
                beatAnalyzer.notesLen = notes.size();

                beatAnalyzer.set_bpm(bpm);
                beatAnalyzer.set_depth(beatLength);

                beatAnalyzer.get_best_branch_at(noteIndex);
                BeatBranch actualBranch = beatAnalyzer.bestBranch;

                BeatBranch expectedBranch = create_branch(noteIndex, beatLength, true);

                if (expectedBranch.length != beatAnalyzer.bestBranch.length)
                {
                    failStatus = true;
                    /* errorMsg += "- Branch lengths don't match:\n\tExpected: " +
                                std::to_string(expectedBranch.length) +
                                "\n\tActual: " + std::to_string(beatAnalyzer.bestBranch.length) +
                                "\n\n"; */
                }

                float prevBaseDuration = 0.f;

                for (unsigned int i = 0; i < expectedBranch.length; i++)
                {
                    auto expectedBeat = expectedBranch.beatBuffer[i];

                    auto actualBeat = beatAnalyzer.bestBranch.beatBuffer[i];

                    std::string beatCmpErrorMsg = compare_beats(expectedBeat, actualBeat);
                    if (beatCmpErrorMsg != "")
                    {
                        failStatus = true;
                        /* errorMsg += beatCmpErrorMsg; */
                    }

                    float baseDuration = beatAnalyzer.bestBranch.beatBuffer[i].duration /
                                         beatAnalyzer.bestBranch.beatBuffer[i].division.consequent;

                    if (i > 0 && beatAnalyzer.bestBranch.beatBuffer[i - 1].division.consequent !=
                                     beatAnalyzer.bestBranch.beatBuffer[i].division.consequent)
                    {
                        float durationDiff = std::abs(baseDuration - prevBaseDuration);
                        float expectedDiff =
                            std::abs(beatAnalyzer.bestBranch.beatBuffer[i - 1].duration /
                                     beatAnalyzer.bestBranch.beatBuffer[i].division.consequent);
                        /* std::cout << "i = " << i << '\n';
                        std::cout << "beat1: " << beatAnalyzer.bestBranch.beatBuffer[i - 1].str()
                                  << '\n';
                        std::cout << "beat2: " << beatAnalyzer.bestBranch.beatBuffer[i].str()
                                  << '\n';
                        std::cout << "durationDiff = " << durationDiff
                                  << ", expectedDiff = " << expectedDiff << '\n'; */
                    }

                    prevBaseDuration = baseDuration;
                }

                /* failStatus = true; */

                if (failStatus)
                {
                    std::cout << errorMsg << '\n';

                    std::cout << "- Expected data:\n";
                    for (unsigned int i = 0; i < expectedBranch.length; i++)
                    {
                        std::cout << "\t" << expectedBranch.dataBuffer[i].str() << '\n';
                    }
                    std::cout << "- Actual data:\n";
                    for (unsigned int i = 0; i < beatAnalyzer.bestBranch.length; i++)
                    {
                        std::cout << "\t" << beatAnalyzer.bestBranch.dataBuffer[i].str() << '\n';
                    }
                    std::cout << "- Expected beats:\n";
                    for (unsigned int i = 0; i < expectedBranch.length; i++)
                    {
                        expectedBranch.beatBuffer[i].calc_note_partials();
                        expectedBranch.beatBuffer[i].calc_score();
                        std::cout << "\t" << expectedBranch.beatBuffer[i].str() << '\n';

                        auto beat = expectedBranch.beatBuffer[i];
                        float distScoreSum = 0.f;
                        float distWeightSum = 0.f;
                        for (unsigned int j = 0; j < beat.notesLen; j++)
                        {
                            float baseDuration = beat.noteRatios[j].antecedent *
                                                 ((60.f / bpm) / beat.division.consequent);

                            float scoreBase = beat.notes[j].duration / baseDuration;

                            /// Humans aren't as good at timing longer notes than they are shorter
                            /// notes (I think), unless the notes are repeated consecutively with a
                            /// metronome. Basically, longer notes leave more room for error, so
                            /// make longer notes have slightly less impact on overall dist score.
                            float noteDistWeight =
                                0.05f / (beat.notes[j].duration * beat.notes[j].duration + 0.05f);

                            distScoreSum +=
                                noteDistWeight * (scoreBase > 1 ? 1.f / scoreBase : scoreBase);
                            distWeightSum += noteDistWeight;

                            /* std::cout
                                << "\t\t- Note " << j << " modified dist score: "
                                << std::to_string(noteDistWeight *
                                                  (scoreBase > 1 ? 1.f / scoreBase : scoreBase))
                                << "/" << noteDistWeight << '\n'; */
                        }
                        /* std::cout << "\t\t- Weighted average: "
                                  << std::to_string(distScoreSum / distWeightSum) << '\n'; */
                    }
                    std::cout << "- Actual beats:\n";
                    for (unsigned int i = 0; i < beatAnalyzer.bestBranch.length; i++)
                    {
                        std::cout << "\t" << beatAnalyzer.bestBranch.beatBuffer[i].str() << '\n';

                        auto beat = beatAnalyzer.bestBranch.beatBuffer[i];
                        float distScoreSum = 0.f;
                        float distWeightSum = 0.f;
                        for (unsigned int j = 0; j < beat.notesLen; j++)
                        {
                            float baseDuration = beat.noteRatios[j].antecedent *
                                                 ((60.f / bpm) / beat.division.consequent);

                            float scoreBase = beat.notes[j].duration / baseDuration;

                            /// Humans aren't as good at timing longer notes than they are shorter
                            /// notes (I think), unless the notes are repeated consecutively with a
                            /// metronome. Basically, longer notes leave more room for error, so
                            /// make longer notes have slightly less impact on overall dist score.
                            float noteDistWeight =
                                0.05f / (beat.notes[j].duration * beat.notes[j].duration + 0.05f);

                            distScoreSum +=
                                noteDistWeight * (scoreBase > 1 ? 1.f / scoreBase : scoreBase);
                            distWeightSum += noteDistWeight;

                            /* std::cout
                                << "\t\t- Note " << j << " modified dist score: "
                                << std::to_string(noteDistWeight *
                                                  (scoreBase > 1 ? 1.f / scoreBase : scoreBase))
                                << "/" << noteDistWeight << '\n'; */
                        }
                        /* std::cout << "\t\t- Weighted average: "
                                  << std::to_string(distScoreSum / distWeightSum) << '\n'; */
                    }

                    beatAnalyzer.branch = beatAnalyzer.bestBranch;
                    float actualScore = beatAnalyzer.calc_branch_score();

                    beatAnalyzer.branch = expectedBranch;
                    float expectedScore = beatAnalyzer.calc_branch_score();

                    std::cout << "- Expected score: " << expectedScore << '\n';
                    std::cout << "- Actual score: " << actualScore << '\n';
                }
                else
                {
                    std::cout << prefix << "Passed\n";
                }

                return failStatus;
            }

            BeatBranch create_branch(unsigned int noteIndex, unsigned int beatLength,
                                     bool createBeatBuffer = false)
            {
                BeatBranch branch;

                branch.dataBuffer[0] = BeatData{&(notes[noteIndex]), 0, false, false};
                branch.length = beatLength;

                /* Beat beat = Beat(&(notes[noteIndex]), 0); */
                Beat beat = Beat(&(notes[noteIndex]));

                unsigned int dataIndex = 0;

                for (unsigned int i = noteIndex; i < notes.size(); i++)
                {
                    auto noteRatio = NoteRatio{noteRatios[i].antecedent, noteRatios[i].consequent};

                    beat.add_note(NoteRatio{noteRatios[i].antecedent, noteRatios[i].consequent});

                    /* std::cout << "beat after adding " << noteRatio.antecedent << "/"
                              << noteRatio.consequent << ": " << beat.str() << '\n'; */

                    if (beat.division.antecedent == beat.division.consequent)
                    {
                        branch.dataBuffer[dataIndex].length = beat.notesLen;

                        if (createBeatBuffer)
                        {
                            branch.beatBuffer[dataIndex] = beat;
                        }

                        if (i == notes.size() - 1)
                        {
                            branch.length = dataIndex + 1;

                            break;
                        }

                        if (dataIndex == beatLength - 1)
                            break;

                        dataIndex++;

                        branch.dataBuffer[dataIndex] = BeatData{&(notes[i + 1]), 0, false, false};

                        beat = beat.create_next();
                    }
                    if (beat.division.antecedent > beat.division.consequent)
                    {
                        branch.dataBuffer[dataIndex].length = beat.notesLen;
                        branch.dataBuffer[dataIndex].endsOffbeat = true;

                        if (createBeatBuffer)
                        {
                            branch.beatBuffer[dataIndex] = beat;
                        }

                        if (dataIndex == beatLength - 1)
                            break;

                        dataIndex++;

                        branch.dataBuffer[dataIndex] = BeatData{&(notes[i + 1]), 0, true, false};

                        beat = beat.create_next();
                    }
                }

                return branch;
            }

        private:
            int note_index_at_timestamp(float timestamp) const
            {
                for (int i = 0; i < notes.size(); i++)
                {
                    if (std::abs(notes[i].timestamp - timestamp) < 0.0001f)
                    {
                        return i;
                    }
                }
                return 0;
            }

            /// @brief
            /// @param expected
            /// @param actual
            /// @return Error message
            std::string compare_beats(Beat expectedBeat, Beat actualBeat)
            {
                std::string errorMsg = "";

                if (expectedBeat.notesLen != actualBeat.notesLen)
                {
                    errorMsg += "- Beat note length doesn't match:\n\tExpected: " +
                                std::to_string(expectedBeat.notesLen) +
                                "\n\tActual: " + std::to_string(actualBeat.notesLen) + "\n\n";
                }

                if (expectedBeat.startTime != actualBeat.startTime ||
                    expectedBeat.endTime != actualBeat.endTime)
                {
                    errorMsg += "- Beat time interval doesn't match:\n\tExpected: " +
                                std::to_string(expectedBeat.startTime) + "->" +
                                std::to_string(expectedBeat.endTime) +
                                "\n\tActual: " + std::to_string(actualBeat.startTime) + "->" +
                                std::to_string(actualBeat.endTime) + "\n\n";
                }

                auto expectedDivision = expectedBeat.division;
                expectedDivision.simplify();

                auto actualDivision = actualBeat.division;
                actualDivision.simplify();

                if (expectedDivision.antecedent != actualDivision.antecedent ||
                    expectedDivision.consequent != actualDivision.consequent)
                {
                    errorMsg += "- Beat division doesn't match:\n\tExpected: " +
                                std::to_string(expectedDivision.antecedent) + "/" +
                                std::to_string(expectedDivision.consequent) +
                                "\n\tActual: " + std::to_string(actualDivision.antecedent) + "/" +
                                std::to_string(actualDivision.consequent) + "\n\n";
                }

                /// Compare notes
                for (unsigned int i = 0; i < expectedBeat.notesLen; i++)
                {
                    /// Note ratio consequents may be removed eventually, so we're using the
                    /// consequent of division instead.
                    auto expectedRatio = NoteRatio{expectedBeat.noteRatios[i].antecedent,
                                                   expectedBeat.division.consequent};
                    expectedRatio.simplify();

                    auto actualRatio = NoteRatio{actualBeat.noteRatios[i].antecedent,
                                                 actualBeat.division.consequent};
                    actualRatio.simplify();

                    if (expectedRatio.antecedent != actualRatio.antecedent ||
                        expectedRatio.consequent != actualRatio.consequent)
                    {
                        errorMsg += "- Beat note ratio doesn't match at note index " +
                                    std::to_string(i) +
                                    ":\n\tExpected: " + std::to_string(expectedRatio.antecedent) +
                                    "/" + std::to_string(expectedRatio.consequent) +
                                    "\n\tActual: " + std::to_string(actualRatio.antecedent) + "/" +
                                    std::to_string(actualRatio.consequent) + "\n\n";
                    }
                }

                return errorMsg;
            }
        };

        void test_transcription(Transcription::Transcription &transcription)
        {
            std::vector<Beat> beats;

            Beat beat = Beat(reinterpret_cast<BaseNote *>(&(transcription.notes[0])), 0);

            std::cout << "note 1 timestamp: " << beat.notes[0].timestamp
                      << ", duration: " << beat.notes[0].duration << '\n';

            /* for (unsigned int i = 0; i < transcription.notes.size(); i++)
            {

                beat.division.simplify();
            } */
        }

        std::vector<BaseNote> timestamps_to_notes(float *timestamps, unsigned int timestampsLen)
        {
            std::vector<BaseNote> notes = std::vector<BaseNote>(timestampsLen, BaseNote{});
            for (unsigned int i = 0; i < timestampsLen - 1; i++)
            {
                notes[i] = BaseNote{timestamps[i], timestamps[i + 1] - timestamps[i]};
            }
            notes[timestampsLen - 1] = BaseNote{timestamps[timestampsLen - 1], -1.f};

            return notes;
        }

        void run_beat_score_tests()
        {
            auto testSource = TestSource("rhythm X 2022");
            testSource.test_beat_scores(0U, 4);
            testSource.test_beat_scores(19U, 4);
            testSource.test_beat_scores(32U, 4);
            testSource.test_beat_scores(44U, 3);

            testSource = TestSource("src jam");
            testSource.test_beat_scores(0U, 4);

            testSource = TestSource("bc 2017");
            testSource.test_beat_scores(0U, 4);

            /* testSource = TestSource("bd 2017");
            testSource.test_beat_scores(30U, 4);
            testSource.test_beat_scores(10U, 4); */

            /* testSource = TestSource("bd 2022");
            testSource.test_beat_scores(0U, 3);
            testSource.test_beat_scores(7U, 4);
            testSource.test_beat_scores(42U, 2); */
        }

        void run_branch_score_tests()
        {
            TestSource testSource;

            /// Tests ability to handle notes with duration longer than a quarter note + shortening
            /// depth if the last note is reached.
            testSource = TestSource("computer\\3_5 beat pause downbeat");
            testSource.test_branch_scores(0U, 8U, 120.f);
            testSource = TestSource("computer\\3_5 beat pause offbeat");
            testSource.test_branch_scores(0U, 8U, 120.f);

            testSource = TestSource("computer\\rhythm X 2022");
            testSource.test_branch_scores(0U, 8U, 160.f);
            testSource.test_branch_scores(70U, 5U, 160.f);

            testSource = TestSource("rhythm X 2022");
            testSource.test_branch_scores(0U, 4U, 164.f);
            testSource.test_branch_scores(0U, 5U, 164.f);
            testSource.test_branch_scores(0U, 6U, 164.f);
            testSource.test_branch_scores(0U, 7U, 164.f);
            testSource.test_branch_scores(0U, 8U, 164.f);
            testSource.test_branch_scores(13U, 6U, 164.f);

            /// These two should pass some time in the future, they are currently expected to fail.
            /* testSource.test_branch_scores(38U, 3U, 164.f);
            testSource.test_branch_scores(43U, 3U, 164.f); */

            testSource = TestSource("computer\\rhythm X 2022");
            testSource.test_branch_scores(0U, 6U, 164.f);

            testSource = TestSource("src jam");
            testSource.test_branch_scores(0U, 6U, 190.f);

            /// This case fails with expected score being higher than the actual, which means the
            /// true branch isn't being considered. The mistake is also minor, and when looking at
            /// note durations it makes sense. It also gets back on to the correct beat afterwards.
            testSource.test_branch_scores(22U, 6U, 190.f);

            testSource = TestSource("bc 2017");
            testSource.test_branch_scores(0U, 6U, 132.f);

            testSource = TestSource("bd 2022");
            testSource.test_branch_scores(0U, 6U, 125.f);
            testSource.test_branch_scores(0U, 8U, 125.f);
        }

        LONGLONG run_benchmark_for(std::string source, float bpm, unsigned int startNoteIndex,
                                   unsigned int depth)
        {
            auto testSource = TestSource(source);

            StartTimer();

            BeatAnalyzer beatAnalyzer;

            beatAnalyzer.set_depth(8);
            beatAnalyzer.set_bpm(bpm);

            beatAnalyzer.notes = &(testSource.notes[0]);
            beatAnalyzer.notesLen = testSource.notes.size();

            beatAnalyzer.get_best_branch_at(0);

            StopTimer(false);

            auto elapsedTime = GetElapsedTime();

            std::cout << "Time taken for source \"" << source << "\" (" << startNoteIndex << ", "
                      << depth << "): " << format_time(elapsedTime) << '\n';
            /* std::cout << "Iterations: " << RhythmTranscriber::iters << '\n';

            RhythmTranscriber::iters = 0; */

            return elapsedTime;
        }
    }

    void run_tests()
    {
        /* run_beat_score_tests(); */
        StartTimer();
        run_branch_score_tests();
        StopTimer();
    }

    void run_benchmarks()
    {
        LONGLONG totalTime = 0;

        /// Default compile options, compiled to executable and run externally.

        /// Average runtime (pre-optimization): 1.38 seconds
        totalTime += run_benchmark_for("rhythm X 2022", 164.f, 0, 8);
        /// Average runtime (pre-optimization): 2.89 seconds
        totalTime += run_benchmark_for("computer\\rhythm X 2022", 164.f, 0, 8);
        /// Average runtime (pre-optimization): 9.89 seconds
        totalTime += run_benchmark_for("bd 2022", 125.f, 0, 8);

        std::cout << "Total time taken for all benchmarks: " << format_time(totalTime) << '\n';
    }
}