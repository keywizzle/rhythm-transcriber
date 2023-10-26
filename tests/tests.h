#pragma once

#include "../Beat.h"
#include "../BeatAnalyzer.h"
#include "../BeatBranch.h"
#include "../transcription/TranscriptionFile.h"
#include <iostream>

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
                beatAnalyzer.maxDepth = beatLength;

                beatAnalyzer.notes = &(notes[0]);
                beatAnalyzer.notesLen = notes.size();
                beatAnalyzer.bpm = bpm;

                beatAnalyzer.get_best_branch_at(noteIndex);

                BeatBranch expectedBranch = create_branch(noteIndex, beatLength, true);

                if (expectedBranch.length != beatAnalyzer.bestBranch.length)
                {
                    failStatus = true;
                    errorMsg += "- Branch lengths don't match:\n\tExpected: " +
                                std::to_string(expectedBranch.length) +
                                "\n\tActual: " + std::to_string(beatAnalyzer.bestBranch.length) +
                                "\n\n";
                }

                for (unsigned int i = 0; i < expectedBranch.length; i++)
                {
                    auto expectedBeat = expectedBranch.beatBuffer[i];

                    auto actualBeat = beatAnalyzer.bestBranch.beatBuffer[i];

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
                    }
                    std::cout << "- Actual beat:\n";
                    for (unsigned int i = 0; i < beatAnalyzer.bestBranch.length; i++)
                    {
                        std::cout << "\t" << beatAnalyzer.bestBranch.beatBuffer[i].str() << '\n';
                    }
                }
                else
                {
                    std::cout << prefix << "Passed\n";
                }

                return failStatus;
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
            BeatBranch create_branch(unsigned int noteIndex, unsigned int beatLength,
                                     bool createBeatBuffer = false)
            {
                BeatBranch branch;

                branch.dataBuffer[0] = BeatData{&(notes[noteIndex]), 0, false, false};
                branch.length = beatLength;

                Beat beat = Beat(&(notes[noteIndex]), 0);

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

            testSource = TestSource("src jam");
            testSource.test_beat_scores(0U, 4);

            testSource = TestSource("bc 2017");
            testSource.test_beat_scores(0U, 4);

            testSource = TestSource("bd 2017");
            testSource.test_beat_scores(30U, 4);
            testSource.test_beat_scores(10U, 4);

            /* testSource = TestSource("bd 2022");
            testSource.test_beat_scores(0U, 3);
            testSource.test_beat_scores(7U, 4);
            testSource.test_beat_scores(42U, 2); */
        }

        void run_branch_score_tests()
        {
            auto testSource = TestSource("rhythm X 2022");
            testSource.test_branch_scores(0U, 4U, 164.f);
            testSource.test_branch_scores(19U, 4U, 164.f);
            /* testSource.test_branch_scores(38U, 2U, 164.f); */
            /* testSource.test_branch_scores(43U, 8U, 164.f); */

            testSource = TestSource("bd 2017");
            /* testSource.test_branch_scores(21U, 4, 170.f); */
            testSource.test_branch_scores(10U, 4, 160.f);

            /* testSource = TestSource("bd 2022");
            testSource.test_branch_scores(0U, 4, 132.f); */
        }
    }
    void run_tests()
    {
        /// Types of tests:
        /// - Full transcription on own (soon)
        /// - Full transcription with given BPM (soon)
        /// - Selected beat branches with given BPM
        /// - Rhythms of selected beats

        run_beat_score_tests();
        run_branch_score_tests();
    }
}