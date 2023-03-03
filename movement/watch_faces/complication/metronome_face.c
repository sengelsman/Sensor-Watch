/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "metronome_face.h"
#include "watch.h"

#define METRONOME_TICK_FREQUENCY 64

static const int8_t _sound_first_beat[] = {BUZZER_NOTE_G7, 1, 0};
static const int8_t _sound_beat[] = {BUZZER_NOTE_G6, 1, 0};

void metronome_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(metronome_face_state_t));
        memset(*context_ptr, 0, sizeof(metronome_face_state_t));
    }
}

static uint8_t _metronome_face_calculate_beat_tick(metronome_face_state_t *state) {
    return (uint8_t)roundf((METRONOME_TICK_FREQUENCY / ((float)state->bpm / 60))) - 1;
}

void metronome_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    metronome_face_state_t *state = (metronome_face_state_t *)context;
    state->bpm = 120;
    state->ticks = 0;
    state->beat_interval = _metronome_face_calculate_beat_tick(state);
    state->counter = 0;
    state->active = false;
}

static void _metronome_face_update_lcd(metronome_face_state_t *state) {
    char buf[11];
    if (state->active) {
        sprintf(buf, "ME- %i ", state->bpm);
    } else {
        sprintf(buf, "ME  %i ", state->bpm);
    }
    watch_display_string(buf, 0);
}

bool metronome_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    metronome_face_state_t *state = (metronome_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _metronome_face_update_lcd(state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            state->bpm++;
            state->beat_interval = _metronome_face_calculate_beat_tick(state);
            _metronome_face_update_lcd(state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            state->bpm += -10;
            state->beat_interval = _metronome_face_calculate_beat_tick(state);
            _metronome_face_update_lcd(state);
            break;
       case EVENT_ALARM_BUTTON_UP:
            if (!state->active) {
                movement_request_tick_frequency(METRONOME_TICK_FREQUENCY); 
                state->active = true;
                _metronome_face_update_lcd(state);
            } else {
                state->active = false;
                movement_request_tick_frequency(1); 
                state->counter = 0;
                state->ticks = 0;
                _metronome_face_update_lcd(state);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->bpm += 10;
            state->beat_interval = _metronome_face_calculate_beat_tick(state);
            _metronome_face_update_lcd(state);
            break;
       case EVENT_TICK:
            //if (event.subsecond > 0) printf("event.subsecond: %d\n", event.subsecond);
            if (state->active) {
                    //printf("state->beat_interval: %d - state->ticks: %d - event.subsecond: %d\n", state->beat_interval, state->ticks, event.subsecond);
                state->ticks++;
                if (state->ticks == 1) {
                    if (state->counter != 0) {
                        // Non UI blocking function to play buzzer sequences
                        watch_buzzer_play_sequence((int8_t *)_sound_beat, NULL);
                    } else {       
                        watch_buzzer_play_sequence((int8_t *)_sound_first_beat, NULL);
                    }
                    state->counter++;
                } else {
                    if (state->ticks == state->beat_interval) {
                        state->ticks = 0;
                    }
                if (state->counter == 4) {
                    state->counter = 0;
                    }
                }
            }
            break;
        case EVENT_TIMEOUT:
            if (!state->active) movement_move_to_face(0);
            break;
        default:
            movement_default_loop_handler(event, settings);
            break;
    }
    return true;
}

void metronome_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}
