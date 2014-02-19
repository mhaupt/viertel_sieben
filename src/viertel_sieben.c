/*
 * This material is distributed under the GNU General Public License
 * Version 2. You may review the terms of this license at
 * http://www.gnu.org/licenses/gpl-2.0.html
 *
 * Copyright (C) 2013, Michael Haupt <pebble at haupz dot de>
 *
 * All rights reserved.
 */

//
// A Pebble watchface displaying time in the way that is used in some parts of Germany.
//
// Rules:
// * a "point in time" is a 5-minute mark on the clock face
// * two to one minutes before a point in time is "gleich"
// * the minute of a point in time is not expressed in a fuzzy way
// * one to two minutes after a point in time is "gerade"
// * the dominant hour is X from the points in time (X-1):15 to X:10
//

#include <pebble.h>

static Window *w;
static TextLayer *t;

//
// text data
//

const char *T_HOURS[] = {
    "zwölf",
    "eins",
    "zwei",
    "drei",
    "vier",
    "fünf",
    "sechs",
    "sieben",
    "acht",
    "neun",
    "zehn",
    "elf"
};

#define PIT_FULL 0
#define PIT_VIERTEL 3

const char *T_POINTS_IN_TIME[] = {
    "", /* 0, PIT_FULL */
    "fünf nach",
    "zehn nach",
    "viertel", /* 3, PIT_VIERTEL */
    "zehn vor halb",
    "fünf vor halb",
    "halb",
    "fünf nach halb",
    "zehn nach halb ",
    "dreiviertel",
    "zehn vor",
    "fünf vor"
};

const char *T_FUZZY[] = {
    "gleich",
    "",
    "gerade"
};

//
// tick handling
//

int compare(int x) {
    return x == 0 ? 0 : (x < 0 ? -1 : 1);
}

#define TIME_STRING_LENGTH 32

#define T_MIN (tt->tm_min)
#define T_HR (tt->tm_hour)

#define IS_FULL (pit == PIT_FULL)

void tick(struct tm *tt, TimeUnits tu) {
    static char time_string[TIME_STRING_LENGTH];
    
    // determine point in time (0: full hour, 11: X:55)
    int pit = ((T_MIN + 2) % 60) / 5;

    // determine dominant hour
    int dhr = (pit >= PIT_VIERTEL || T_MIN >= 58 ? T_HR + 1 : T_HR) % 12;

    // determine fuzzyness
    int fuzzy = T_MIN == 58 || T_MIN == 59 ? 0 : compare(T_MIN - pit * 5) + 1;

    // assemble string
    memset(time_string, 0, TIME_STRING_LENGTH);
    snprintf(time_string, TIME_STRING_LENGTH, "%s%s%s%s\n%s", IS_FULL ? "\n" : "", T_FUZZY[fuzzy],
        IS_FULL ? "" : "\n", T_POINTS_IN_TIME[pit], T_HOURS[dhr]);

    text_layer_set_text(t, time_string);
}

//
// setup, shutdown, and main
//

void setup(void) {
    w = window_create();
    window_stack_push(w, true);
    window_set_background_color(w, GColorBlack);

    Layer *wl = window_get_root_layer(w);

    t = text_layer_create(GRect(2, 8, 142, 160));
    text_layer_set_text_color(t, GColorWhite);
    text_layer_set_background_color(t, GColorClear);
    text_layer_set_font(t, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    layer_add_child(wl, text_layer_get_layer(t));

    tick_timer_service_subscribe(MINUTE_UNIT, tick);
}

void shutdown(void) {
    tick_timer_service_unsubscribe();
}

int main(void) {
    setup();
    app_event_loop();
    shutdown();
}

