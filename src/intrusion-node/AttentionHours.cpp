#include "AttentionHours.h"
#include "Config.h"

bool isOutsideAttentionHours(const time_t utcInstant) {
    const time_t localInstant = utcInstant + ATTENTION_UTC_OFFSET_IN_SECONDS;
    tm localTime{};
    gmtime_r(&localInstant, &localTime);
    return localTime.tm_hour < ATTENTION_OPENING_HOUR || localTime.tm_hour >= ATTENTION_CLOSING_HOUR;
}