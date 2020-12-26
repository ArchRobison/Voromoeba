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

#include "Background.h"
#include "NimbleDraw.h"
#include "Help.h"
#include "Region.h"
#include "Widget.h"

static Background HelpBackground;
static ReducedAngle Theta;
static ViewTransform HelpViewTransform;

void Help::initialize(NimblePixMap& window) {
    Circle windowCircle = Circle(Point(0, 0), std::sqrt(Dist2(Center(window))));
    HelpBackground.initialize(window, 200, [&](Point& p)->bool {
        p = windowCircle.radius()*(Point(RandomFloat(2), RandomFloat(2))-Point(1, 1));
        return windowCircle.contains(p);
        });
    HelpViewTransform.setScaleAndRotation(1, Theta);
    HelpViewTransform.setOffset(0.5f*Point(window.width(), window.height()));
}

void Help::update(float dt, float deltaTheta) {
    Theta += deltaTheta;
    HelpViewTransform.setRotation(Theta);
}

// The original plan was to use SVG, but SDL does not render SVG text.
// So instead the resources include two pre-rendedere .png files.

//! Overlays in increasing size.
static InkOverlay TheHelp[2] = {"Help.934x633.png","Help.1867x1265.png"};

void Help::draw(NimblePixMap& window) {
    // Draw background
    CompoundRegion region;
    region.buildRectangle(Point(0, 0), Point(window.width(), window.height()));
    Ant* a = Ant::openBuffer();
    a = HelpBackground.copyToAnts(a, HelpViewTransform);
    Ant::closeBufferAndDraw(window, region, a, true);

    // Find biggest available help overlay that fits screen.
    auto* h= TheHelp + 1;
    while (h > TheHelp && (h->width() > window.width() || h->height() > window.height())) 
        --h;
    h->drawOn(window, Max((window.width()-h->width())/2, 0), Max((window.height()-h->height())/2, 0));
}