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

#include "About.h"
#include "Background.h"
#include "Bug.h"
#include "Enum.h"
#include "NimbleDraw.h"
#include "Region.h"
#include "VoronoiText.h"
#include "Widget.h"
#include <cmath>

namespace {

//! Widget for displaying Voronoi diagram colored according to an underlying photo.
class Photo : public Widget {
    BugArray<Bug> myBugs;
public:
    Photo(const char* resourceName) : Widget(resourceName) {}
    void initialize();
    void update(float dt);
    Ant* copyToAnts(Ant* a, const ViewTransform& transform);
    int width() const { return myPixMap.width(); }
    int height() const { return myPixMap.height(); }
};

void Photo::initialize() {
    size_t m = 4096;
    myBugs.reserve(m);
    for (size_t k=0; k<m; ++k) {
        auto& b = myBugs[k];
        b.pos.x = RandomFloat(myPixMap.width());
        b.pos.y = RandomFloat(myPixMap.height());
        b.vel = Polar(1.f, RandomAngle());
    }
}

void Photo::update(float dt) {
    for (size_t k=0; k<myBugs.size(); ++k) {
        auto& b = myBugs[k];
        b.pos += dt*b.vel;
        if (b.pos.x<0 && b.vel.x<0 || b.pos.x>=myPixMap.width() && b.vel.x>0)
            b.vel.x *= -1;
        if (b.pos.y<0 && b.vel.y<0 || b.pos.y>=myPixMap.height() && b.vel.y>0)
            b.vel.y *= -1;
        float x = Clip<float>(0, myPixMap.width()-1, b.pos.x);
        float y = Clip<float>(0, myPixMap.height()-1, b.pos.y);
        b.color = myPixMap.interpolatePixelAt(x, y);
    }
}

Ant* Photo::copyToAnts(Ant* a, const ViewTransform& transform) {
    return myBugs.copyToAnts(a, transform);
}

Photo TheAuthor("Author");
VoronoiText TheTitle;
VoronoiText TheInfo;
enum class RectIndex : int8_t { author, title, info };

} // (anonymous)

template<>
constexpr RectIndex EnumMax<RectIndex> = RectIndex::info; 

namespace {

EnumMap<RectIndex, NimbleRect> Rect;

void ShrinkWrapTextRect(NimbleRect& rect, const VoronoiText& text) {
    float sx = float(rect.width())/text.width();
    float sy = float(rect.height())/text.height();
    float scale = Min(sx, sy)*.9f;
    Point upperLeft(rect.left +0.5f*(rect.width() -scale*text.width()),
        rect.top+0.5f*(rect.height()-scale*text.height()));
    rect.top = Round(upperLeft.y);
    rect.left = Round(upperLeft.x);
    rect.right = Round(upperLeft.x+scale*text.width());
    rect.bottom = Round(upperLeft.y+scale*text.height());
}

static Background AboutBackground;

} // namespace

void About::initialize(NimblePixMap& window) {
    TheAuthor.initialize();
    TheTitle.initialize(
        "Voromoeba 1.2      \n"
        "Copyright 2011-2020\n"
        "Arch D. Robison    ");
    TheInfo.initialize(
        "See http://www.blonzonics.us/games\n"
        "for Voromoeba, Frequon Invaders,  \n"
        "Ecomunch, and Seismic Duck.       "
    );
    Rect[RectIndex::author] = NimbleRect(0, 0, TheAuthor.width(), TheAuthor.height());
    Rect[RectIndex::title] = NimbleRect(Rect[RectIndex::author].right, 0, window.width(), Rect[RectIndex::author].height());
    Rect[RectIndex::info] = NimbleRect(0, Rect[RectIndex::author].bottom, window.width(), window.height());

    ShrinkWrapTextRect(Rect[RectIndex::title], TheTitle);
    ShrinkWrapTextRect(Rect[RectIndex::info], TheInfo);

    AboutBackground.initialize(window, 1024, [&](Point& pos)->bool {
        NimblePoint p;
        p.x = rand() % window.width();
        p.y = rand() % window.height();
        for (const auto& r : Rect)
            if (r.contains(p))
                return false;
        pos.x = p.x;
        pos.y = p.y;
        return true;
        });
}

void About::update(float dt) {
    TheAuthor.update(dt);
}

static Ant* AssignAntsToFit(const NimbleRect& rect, VoronoiText& text, Ant* a) {
    float scale = float(rect.width())/text.width();
    Point upperLeft(rect.left, rect.top);
    return text.copyToAnts(a, upperLeft, scale);
}

void About::draw(NimblePixMap& window) {
    const int32_t wa = TheAuthor.width();
    const int32_t ha = TheAuthor.height();
    NimbleRect titleRect(wa, 0, window.width(), ha);
    NimbleRect infoRect(0, ha, window.width(), window.height());
    CompoundRegion region;
    region.buildRectangle(Point(0, 0), Point(window.width(), window.height()));

    Ant* a = Ant::openBuffer();
    ViewTransform identity;
    a = TheAuthor.copyToAnts(a, identity);
    a = AssignAntsToFit(Rect[RectIndex::title], TheTitle, a);
    a = AssignAntsToFit(Rect[RectIndex::info], TheInfo, a);
    a = AboutBackground.copyToAnts(a, identity);
    Ant::closeBufferAndDraw(window, region, a, true);
}