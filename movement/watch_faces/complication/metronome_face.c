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

void metronome_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(metronome_face_state_t));
        memset(*context_ptr, 0, sizeof(metronome_face_state_t));
    }
}

static uint8_t _metronome_face_calculate_beat_tick(metronome_face_state_t *state) {
    return (uint8_t)roundf((METRONOME_TICK_FREQUENCY / ((float)state->bpm / 60)));
}

void metronome_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    metronome_face_state_t *state = (metronome_face_state_t *)context;
    state->bpm = 120;
    state->ticks = 1;
    state->beat_tick = _metronome_face_calculate_beat_tick(state);
    state->counter = 0;
    state->active = false;
}

static void _metronome_face_update_lcd(metronome_face_state_t *state) {
    char buf[11];
    if (state->active) {
        sprintf(buf, "ME--%i ", state->bpm);
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
            state->beat_tick = _metronome_face_calculate_beat_tick(state);
            _metronome_face_update_lcd(state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            state->bpm += -10;
            state->beat_tick = _metronome_face_calculate_beat_tick(state);
            _metronome_face_update_lcd(state);
            break;
       case EVENT_ALARM_BUTTON_UP:
            if (!state->active) {
                state->active = true;
                movement_request_tick_frequency(METRONOME_TICK_FREQUENCY); 
                _metronome_face_update_lcd(state);
            } else {
                state->active = false;
                state->counter = 0;
                state->ticks = 1;
                _metronome_face_update_lcd(state);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->bpm += 10;
            state->beat_tick = _metronome_face_calculate_beat_tick(state);
            _metronome_face_update_lcd(state);
            break;
       case EVENT_TICK:
            if (state->active) {
                if (state->ticks == state->beat_tick) {
                    if (state->counter == 0) {
                        watch_buzzer_play_note(BUZZER_NOTE_G7, 20);
                    } else {
                        watch_buzzer_play_note(BUZZER_NOTE_G6, 20);
                    }
                    
                    state->counter++;
                    if (state->counter == 4) state->counter = 0;
                    state->ticks = 1;
                }
                state->ticks++;
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
