#include "amf-overload.h"
#include "context.h"

/* Simple UE-based overload check */
amf_overload_result_t amf_overload_check(ran_ue_t *ran_ue)
{
    amf_overload_result_t res = {AMF_OVERLOAD_OK};

    if (!ran_ue || !ran_ue->amf_ue_ngap_id) {
        ogs_debug("Overload check skipped: no UE context");
        return res;
    }

    ogs_info("Overload check: UE context exists: current ue_count=%u, threshold=%u",
            amf_self()->ue_count, amf_self()->ue_overload_threshold);

    /* Simple global UE count threshold */
    if (amf_self()->ue_count >= (amf_self()->ue_overload_threshold - 1)) {
        ogs_info("Overload detected: current ue_count=%u, threshold=%u",
                amf_self()->ue_count, amf_self()->ue_overload_threshold);
        res.type = AMF_OVERLOAD_REJECT;

        //calculate dynamic backoff time
        uint32_t backoff_base = 20;    // 20s minimum
        res.backoff_time = backoff_base + (rand() % 6); // add random jitter 0–5s
        ogs_info("Decided backoff time: %u seconds", res.backoff_time);

        return res;
    }

    // No overload detected
    ogs_info("No overload detected");
    return res;

}

uint8_t encode_t3346(uint32_t seconds)
{
    uint8_t unit = 0;
    uint8_t value = 0;

    if (seconds == 0) {
        // Special case: "timer not present"
        return 0;
    }

    // lower bound
    if (seconds < 2)
        seconds = 2;

    if (seconds <= 2 * 31) {
        unit = 0;              // 2-second unit
        value = seconds / 2;
    } else if (seconds <= 60 * 31) {
        unit = 1;              // 1-minute unit
        value = seconds / 60;
    } else if (seconds <= 360 * 31) {
        unit = 2;              // deci-hour (6-min) unit
        value = seconds / 360;
    } else {
        unit = 2;              // max (31 × 6 min)
        value = 31;
    }

    ogs_info("T3346 encoded value: unit=%u, value=%u", unit, value);

    // Combine into single byte (bits 8–6 = unit, bits 5–1 = value)
    ogs_info("T3346 encoded octet: 0x%02X", (uint8_t)((unit << 5) | (value & 0x1F)));
    return (uint8_t)((unit << 5) | (value & 0x1F));

}
