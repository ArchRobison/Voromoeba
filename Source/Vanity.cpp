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

#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "AssertLib.h"
#include "Host.h"
#include "Vanity.h"
#include "NimbleDraw.h"	// Need NimbleRandom from here
#include "VoronoiText.h"
#include "Region.h"
#include <cctype>

/** Version field was added in version 1.2.  Field was record count in version 1.1.
    0x102 = version 1.2 */
static unsigned short CurrentVersion = 0x102;

void VanityBoardData::VanityCheckSum::generateFrom( const VanityBoardData& data ) {
	Assert( (char*)&data.checkSum>(char*)&data );
	Assert( ((char*)&data.checkSum-(char*)&data)%2 == 0 );
	memset( digit, 0, sizeof(digit) );
	for( unsigned short* p=(unsigned short*)&data; p<(unsigned short*)&data.checkSum; ++p ) {
		unsigned int carry = *p;
		for( int k=0; k<4; ++k ) {
			carry += 41719*digit[k];
			digit[k] = carry;
			carry >>= 16;
	    }
	    digit[0] ^= carry;
	}
}

const char* VanityBoardData::getVanityBoardPath() {
    return HostGetCommonAppData( "\\Voromoeba\\vanity.dat" );
}

void VanityBoardData::warn( const char* message ) const {
    const char* s = getVanityBoardPath();
    size_t n = strlen(message)+strlen(s)+4;
    char* buffer = new char[n];
    sprintf(buffer, "%s : %s",message,s);
    HostWarning(buffer);
    delete[] buffer;
}

void VanityBoardData::readFromFile() {
	bool okay = false;
    if( const char* path = getVanityBoardPath() ) {
        if( FILE* f = fopen(path,"rb") ) {
            size_t n = fread(this,sizeof(VanityBoardData),1,f);
            if( n==1 ) {
                if( version<=10 )
                    warn("Score file from incompatible Voromoeba 1.1");
                else if( size<=maxSize ) {
                    if( version==CurrentVersion ) {
                        VanityCheckSum expected;
                        expected.generateFrom(*this);
                        okay = memcmp(&expected,&checkSum,sizeof(VanityCheckSum))==0;
                    } else if( version<CurrentVersion ) {
                        warn("Score file from older version of Voromoeba");
                    } else {
                        warn("Score file from newer version of Voromoeba");
                    }
                }
            }
            fclose(f);
            if( !okay )
                warn("Score file corrupted");
        } else {
            warn("Cannot open score file for reading");
        }
    }
	if( !okay ) {
		// Board has been corrupted.  Clear it.
		memset( this, 0, sizeof(*this) );
#if 1
        // Use to create initial file to be distributed in package.
        writeToFile();
#endif
	}
}
	
void VanityBoardData::writeToFile() {
    if( const char* path = getVanityBoardPath() ) {
        if( FILE* f = fopen(path,"wb") ) {
            version = CurrentVersion;
            salt = std::rand() ^ std::rand()<<11 ^ std::rand()<<22;
            checkSum.generateFrom(*this);
            // Write the signed copy
            size_t n = fwrite(this,sizeof(VanityBoard),1,f);
            if( n!=1 ) {
                warn("Cannot write to opened score file");
            }
            fclose(f);
        } else {
            warn("Cannot open score file for writing");
        }
    }
}

size_t VanityBoardData::insert( short score ) {
    // Set i to where score should be inserted
	size_t i;
	for( i=0; i<size; ++i )
		if( score > record[i].score ) 
			break;
    if( i<maxSize ) {
        if( size<maxSize ) 
            ++size;
        // insert new score
		memmove( &record[i+1], &record[i], sizeof(record[0])*(maxSize-1-i) ); 
        memset( record[i].name, 0, VAIN_NAME_MAX );
		record[i].score = score;
	}
    return i;
}

VanityBoard TheVanityBoard;
static VoronoiText TheVanityText;

#if WIZARD_ALLOWED
static void CopyTestCharsToVanityText() {
    int k = 0;
    for (int i=0; i<TheVanityText.textHeight(); ++i)
        for (size_t j=0; j<TheVanityText.textWidth(); ++j) {
            char c;
            if (i<8 && j<16)
                c = i*16+j;
            else {
                do {
                    if (++k>=128) k=0;
                } while (TheVoronoiFont[k].size()==0);
                c = k;
            }
            TheVanityText.setChar(i, j, c);
        }
}
#endif

bool VanityBoard::isCursorBlinkedOn() {
    const double blinkPeriod = 1;
    return std::fmod( HostClockTime(), blinkPeriod ) < blinkPeriod*0.5;
}

void VanityBoard::draw( NimblePixMap& window ) {
#if WIZARD_ALLOWED
    if( myShowTestImage ) {
        CopyTestCharsToVanityText();
        TheVanityText.drawOn( window, (window.width()-TheVanityText.width())/2, (window.height()-TheVanityText.height())/2, true );
        return;
    }
#endif
    TheVanityText.setLine(0,"Top Voromoebas");
    TheVanityText.setLine(1,"");
    TheVanityText.setLine(2,"Score Player");
    for( size_t i=0; i<TheVanityBoard.maxSize; ++i ) {
        const int scoreWidth=5;
		char buffer[scoreWidth+1+VAIN_NAME_MAX+1];
        if( i<size ) {
		    sprintf(buffer,"%5d ",record[i].score);
		    strncpy(buffer+scoreWidth+1,record[i].name,VAIN_NAME_MAX);
            if( i==currentRow && isCursorBlinkedOn() ) {
                Assert( 0<=currentCol && currentCol<VAIN_NAME_MAX );
                Assert( buffer[scoreWidth+1+currentCol]=='\0' );
                buffer[scoreWidth+1+currentCol] = 0x7F;
            }
		    buffer[scoreWidth+1+VAIN_NAME_MAX] = 0;
        } else {
            buffer[0] = '\0';
        }
        TheVanityText.setLine(3+i,buffer);
    }
    float scale = Min( 0.9f*window.width()/TheVanityText.width(), 0.9f*window.height()/TheVanityText.height() );
    // Draw the text centered on the screen
    TheVanityText.drawOn( window, (window.width()-TheVanityText.width()*scale)/2, (window.height()-TheVanityText.height()*scale)/2, scale, true );
}

void VanityBoard::newScore( int score ) {
    int i = insert(score);
    if( i<maxSize ) {
        currentRow = i;
        currentCol = 0;
    }
}

bool VanityBoard::enterNextCharacterOfName( int key ) {
    Assert(size_t(currentRow)<maxSize);
    // Only allow letters, spaces, and periods in a name. 
    if( isalpha(key) || strchr(". ",key)!=NULL ) {
        currentChar() = key;
        if( currentCol+1<VAIN_NAME_MAX ) 
            ++currentCol;
    } else {
        switch( key ) {
            case HOST_KEY_RETURN:
                currentRow = -1;
                currentCol = -1;
                writeToFile();
                return true; 
            case HOST_KEY_BACKSPACE:
                 if( currentCol>0 ) 
                     --currentCol;
                currentChar() = 0;
                break;
            default:
                break;
        }
    }
    return false;
}

#if WIZARD_ALLOWED
void VanityBoard::showTestImage( bool flag ) {
    myShowTestImage = flag;
}
#endif

void InitializeVanity() {
    TheVanityBoard.readFromFile();
    TheVanityText.initialize(13,5+1+VAIN_NAME_MAX);
}