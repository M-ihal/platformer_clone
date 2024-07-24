#ifndef _LEVEL_TRANSITION_H
#define _LEVEL_TRANSITION_H

#include "common.h"
#include "maths.h"
#include "renderer.h"

void setup_level_transition(int32_t world, int32_t level);
void update_level_transition(float64_t delta_time);
void render_level_transition(int32_t width, int32_t height);
bool level_transition_finished(void);

#endif /* _LEVEL_TRANSITION_H */