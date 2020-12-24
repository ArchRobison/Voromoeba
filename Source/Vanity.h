/* Copyright 2011-2020 Arch D. Robison

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <cstddef>
#include <cstdint>
#include <string>
#include "Config.h"
#include "NimbleDraw.h"

constexpr size_t VAIN_NAME_MAX = 20;

//! Vanity board data
/** WARNING: This class is written to disk as raw bits.  Its layout must be maintained between versions. */
class VanityBoardData {
    struct VanityCheckSum {
        uint16_t digit[4];
        void generateFrom(const VanityBoardData& data);
    };
    void warn(const char* message) const;
    static std::string getVanityBoardPath();
public:
    struct VanityRecord {
        uint16_t score;
        char name[VAIN_NAME_MAX];
    };
    //! Maximum number of records allowed
    static const size_t maxSize = 10;
    //! Version number of game
    uint16_t version;
    //! Number of records
    uint16_t size;
    /** Sorted by decreasing score. */
    VanityRecord record[maxSize];
    uint32_t salt;
    //! Checksum. Must be declared as last field.
    VanityCheckSum checkSum;
    void readFromFile();
    void writeToFile();
    // Insert score, null out the name, and return index of where record was inserted.
    // Returns VAIN_RECORD_MAX if score is lower than any other score and board is full.
    size_t insert(short score);
};

class VanityBoard : VanityBoardData {
    //! Row at which name is being entered, or -1 if not entering name.
    int currentRow;
    int currentCol;
#if WIZARD_ALLOWED
    bool myShowTestImage;
#endif
    char& currentChar() {
        return record[currentRow].name[currentCol];
    }
    static bool isCursorBlinkedOn();
public:
    using VanityBoardData::readFromFile;
    VanityBoard() : currentRow(-1), currentCol(-1) {}
    void draw(NimblePixMap& screen);
    /** If score is a high score, insert it in the scores, and make subsequent calls to isEnteringName=true.
        Client should then pass user keystrokes to method enterName.
        Otherwise return false. */
    void newScore(int score);
    bool isEnteringName() const {
        return currentRow>=0;
    }
    /** Enter next character of user's name.  Returns true if name is complete */
    bool enterNextCharacterOfName(int key);
#if WIZARD_ALLOWED
    /** If flag is true, show test characters instead of scores */
    void showTestImage(bool flag);
#endif
};

extern VanityBoard TheVanityBoard;

void InitializeVanity();