#pragma once

#include <time.h>

/**
 * @brief Tells whether an instant is outside the attention hours of the gallery.
 * @param utcInstant Instant in UTC.
 * @return true from 22:00 to 10:00 in Lima time, the window the Edge API uses for MOTION.
 */
bool isOutsideAttentionHours(time_t utcInstant);