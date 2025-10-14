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
    if (amf_self()->ue_count >= amf_self()->ue_overload_threshold) {
        ogs_info("Overload detected: current ue_count=%u, threshold=%u",
                amf_self()->ue_count, amf_self()->ue_overload_threshold);
        res.type = AMF_OVERLOAD_REJECT;

        return res;
    }

    // No overload detected
    ogs_info("No overload detected");
    return res;

}
