/*
 * This material is distributed under the GNU General Public License
 * Version 2. You may review the terms of this license at
 * http://www.gnu.org/licenses/gpl-2.0.html
 *
 * Copyright (C) 2013, 2014, Michael Haupt <pebble at haupz dot de>
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

//
// Set this to 1 to enable Catholic features.
//
#define CATHBIT 1

static Window *w;
static TextLayer *text;

#ifdef CATHBIT
static TextLayer *hour;
#endif

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

#ifdef CATHBIT
const int N_LITURGICAL_HOURS = 7;

const char *T_LITURGICAL_HOURS[] = {
    "Matutin",
    "Laudes",
    "Terz",
    "Sext",
    "Non",
    "Vesper",
    "Komplet",
    "<Fehler>"
};

#define MINUTE_OF_DAY(H, M) ((H) * 60 + (M))

// store the ends of liturgical hours in terms of minutes-of-day
// Matutin 00:00 - 04:59
// Laudes  05:00 - 08:29
// Terz    08:30 - 10:29
// Sext    10:30 - 13:29
// Non     13:30 - 15:59
// Vesper  16:00 - 18:59
// Komplet 19:00 - 23:59
const int LITURGICAL_HOUR_ENDS[] = {
    MINUTE_OF_DAY(4, 59),
    MINUTE_OF_DAY(8, 29),
    MINUTE_OF_DAY(10, 29),
    MINUTE_OF_DAY(13, 29),
    MINUTE_OF_DAY(15, 59),
    MINUTE_OF_DAY(18, 59),
    MINUTE_OF_DAY(23, 59)
};
#endif

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

#ifdef CATHBIT
#define HOUR_STRING_LENGTH 16
#endif

void tick(struct tm *tt, TimeUnits tu) {
    static char time_string[TIME_STRING_LENGTH];
#ifdef CATHBIT
    static char hour_string[HOUR_STRING_LENGTH];
#endif
    
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

    text_layer_set_text(text, time_string);

#ifdef CATHBIT
    // Angelus buzz
    if (T_MIN == 0 && (T_HR == 6 || T_HR == 12 || T_HR == 18)) {
        vibes_double_pulse();
    }

    // determine the current liturgical hour
    int liturgical_hour = 0;
    int minute_of_day = MINUTE_OF_DAY(T_HR, T_MIN);
    for (int i = 0; i < N_LITURGICAL_HOURS; ++i) {
        if (LITURGICAL_HOUR_ENDS[i] > minute_of_day) {
            break;
        }
        ++liturgical_hour;
    }

    // set string
    memset(hour_string, 0, HOUR_STRING_LENGTH);
    snprintf(hour_string, HOUR_STRING_LENGTH, T_LITURGICAL_HOURS[liturgical_hour]);

    text_layer_set_text(hour, hour_string);
#endif
}

//
// setup, shutdown, and main
//

#define TEXT_X 2
#define TEXT_X_EXTEND 140

#define TEXT_TIME_Y 8
#define TEXT_TIME_Y_EXTEND 100

#ifdef CATHBIT
#define TEXT_HOUR_Y 120
#define TEXT_HOUR_Y_EXTEND 30
#endif

void setup_text_layer(TextLayer *tl, const char *font_key, GTextAlignment text_align) {
    text_layer_set_text_color(tl, GColorWhite);
    text_layer_set_background_color(tl, GColorClear);
    text_layer_set_font(tl, fonts_get_system_font(font_key));
    text_layer_set_text_alignment(tl, text_align);
}

void setup(void) {
    w = window_create();
    window_stack_push(w, true);
    window_set_background_color(w, GColorBlack);

    Layer *wl = window_get_root_layer(w);

    text = text_layer_create(GRect(TEXT_X, TEXT_TIME_Y, TEXT_X_EXTEND, TEXT_TIME_Y_EXTEND));
    setup_text_layer(text, FONT_KEY_GOTHIC_28_BOLD, GTextAlignmentLeft);
    layer_add_child(wl, text_layer_get_layer(text));

#ifdef CATHBIT
    hour = text_layer_create(GRect(TEXT_X, TEXT_HOUR_Y, TEXT_X_EXTEND, TEXT_HOUR_Y_EXTEND));
    setup_text_layer(hour, FONT_KEY_GOTHIC_28, GTextAlignmentRight);
    layer_add_child(wl, text_layer_get_layer(hour));
#endif

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

