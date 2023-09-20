#include "Transcription.h"

#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"

#include <iostream>

namespace RhythmTranscriber
{
    namespace Transcription
    {
        bool Transcription::check()
        {
            /// TODO:
            /// - Timestamps are in order (for notes AND beats)
            /// - Every beat ends at the start of the next beat
            /// - Etc.
            return true;
        }
        Transcription Transcription::fromJSON(std::string json)
        {
            /// This will run fastest if the JSON string has whitespace/newlines/etc. removed

            Transcription transcription;

            rapidjson::Document doc;
            doc.Parse(json.c_str());

            /// Check if notes exist
            if (doc.FindMember("notes") != doc.MemberEnd())
            {
                auto notesArr = doc["notes"].GetArray();
                auto notesLen = notesArr.Capacity();
                transcription.notes.reserve(notesLen);
                for (auto i = 0; i < notesLen; i++)
                {
                    NoteElement note = NoteElement{};

                    auto noteObj = notesArr.Begin() + i;
                    auto invalidMember = noteObj->MemberEnd();

                    if (noteObj->FindMember("timestamp") == invalidMember)
                    {
                        // Maybe throw an exception here
                        continue;
                    }

                    note.timestamp = (*noteObj)["timestamp"].GetFloat();

                    if (noteObj->FindMember("hand") != invalidMember)
                    {
                        auto type = (*noteObj)["hand"].GetType();
                        if (type == rapidjson::Type::kNumberType)
                        {
                            note.hand = (NoteHand)(*noteObj)["hand"].GetInt();
                        }
                        else if (type == rapidjson::Type::kStringType)
                        {
                            auto noteHand = (*noteObj)["hand"].GetString();
                            note.hand = noteHand[0] == 'r'   ? NoteHand::HAND_RIGHT
                                        : noteHand[0] == 'l' ? NoteHand::HAND_LEFT
                                        : noteHand[0] == 'b' ? NoteHand::HAND_BOTH
                                                             : NoteHand::HAND_NONE;
                        }
                    }

                    if (noteObj->FindMember("articulation") != invalidMember)
                    {
                        auto type = (*noteObj)["articulation"].GetType();
                        if (type == rapidjson::Type::kNumberType)
                        {
                            note.articulation =
                                (NoteArticulation)(*noteObj)["articulation"].GetInt();
                        }
                        else if (type == rapidjson::Type::kStringType)
                        {
                            auto noteArticulation =
                                (std::string)(*noteObj)["articulation"].GetString();
                            note.articulation = noteArticulation == "tap"
                                                    ? NoteArticulation::ARTICULATION_TAP
                                                : noteArticulation == "tenuto"
                                                    ? NoteArticulation::ARTICULATION_TENUTO
                                                : noteArticulation == "accent"
                                                    ? NoteArticulation::ARTICULATION_ACCENT
                                                    : NoteArticulation::ARTICULATION_NONE;
                        }
                    }

                    if (noteObj->FindMember("placement") != invalidMember)
                    {
                        // TODO: Add all note placements
                        auto type = (*noteObj)["placement"].GetType();
                        if (type == rapidjson::Type::kNumberType)
                        {
                            note.placement = (NotePlacement)(*noteObj)["placement"].GetInt();
                        }
                        else if (type == rapidjson::Type::kStringType)
                        {
                            auto notePlacement = (std::string)(*noteObj)["placement"].GetString();
                            note.placement =
                                notePlacement == "head"       ? NotePlacement::PLACEMENT_HEAD
                                : notePlacement == "rimshot"  ? NotePlacement::PLACEMENT_RIM_SHOT
                                : notePlacement == "rimclick" ? NotePlacement::PLACEMENT_RIM_CLICK
                                                              : NotePlacement::PLACEMENT_NONE;
                        }
                    }

                    if (noteObj->FindMember("rhythm") != invalidMember)
                    {
                        auto rhythmEnd = (*noteObj)["rhythm"].MemberEnd();
                        if ((*noteObj)["rhythm"].FindMember("notes") != rhythmEnd)
                            note.rhythm.notes = (*noteObj)["rhythm"]["notes"].GetUint();
                        if ((*noteObj)["rhythm"].FindMember("beats") != rhythmEnd)
                            note.rhythm.beats = (*noteObj)["rhythm"]["beats"].GetUint();
                        if ((*noteObj)["rhythm"].FindMember("bpm") != rhythmEnd)
                            note.rhythm.bpm = (*noteObj)["rhythm"]["bpm"].GetFloat();
                    }

                    if (noteObj->FindMember("flam") != invalidMember)
                    {
                        note.flam = (*noteObj)["flam"].GetBool();
                    }

                    if (noteObj->FindMember("diddle") != invalidMember)
                    {
                        note.diddle = (*noteObj)["diddle"].GetBool();
                    }

                    if (noteObj->FindMember("buzz") != invalidMember)
                    {
                        note.buzz = (*noteObj)["buzz"].GetBool();
                    }

                    /* std::cout << "timestamp: " << std::to_string(note.timestamp) << '\n';
                    std::cout << "hand: " << std::to_string(note.hand) << '\n';
                    std::cout << "articulation: " << std::to_string(note.articulation) << '\n';
                    std::cout << "rhythm: " << std::to_string(note.rhythm.notes) << "/"
                              << std::to_string(note.rhythm.beats) << '\n'; */
                    transcription.notes.push_back(note);
                }
            }

            // TODO: Get beat data

            return transcription;
        }
        std::string Transcription::toJSON(bool compressed)
        {
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

            writer.StartObject();

            writer.String("notes", 5);

            writer.StartArray();

            auto notesSize = notes.size();
            for (unsigned int i = 0; i < notesSize; i++)
            {
                /// Serialize note (TODO: maybe put this in separate method)
                auto note = notes.at(i);

                writer.StartObject();

                writer.String("timestamp", 9);
                writer.Double(note.timestamp);

                if (!compressed || note.hand != HAND_NONE)
                {
                    writer.String("hand", 4);
                    writer.Int(note.hand);
                }

                if (!compressed || note.articulation != ARTICULATION_NONE)
                {
                    writer.String("articulation", 12);
                    writer.Int(note.articulation);
                }

                if (!compressed || note.placement != PLACEMENT_NONE)
                {
                    writer.String("placement", 9);
                    writer.Int(note.placement);
                }

                if (!compressed || (note.rhythm.notes != 0 || note.rhythm.beats != 0))
                {
                    writer.String("rhythm", 6);
                    writer.StartObject();
                    writer.String("notes", 5);
                    writer.Uint(note.rhythm.notes);
                    writer.String("beats", 5);
                    writer.Uint(note.rhythm.beats);
                    writer.EndObject();
                }

                if (!compressed || note.flam)
                {
                    writer.String("flam", 4);
                    writer.Bool(note.flam);
                }

                if (!compressed || note.diddle)
                {
                    writer.String("diddle", 6);
                    writer.Bool(note.diddle);
                }

                if (!compressed || note.buzz)
                {
                    writer.String("buzz", 4);
                    writer.Bool(note.buzz);
                }

                writer.EndObject();
            }
            writer.EndArray();

            writer.EndObject();

            return sb.GetString();
        }
    }
}