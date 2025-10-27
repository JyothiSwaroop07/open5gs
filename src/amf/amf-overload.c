#include "amf-overload.h"
#include "context.h"
#include "ogs-core.h"

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
    if (amf_self()->ue_count >= (amf_self()->ue_overload_threshold )) {
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


char *s_nssai_key(const ogs_s_nssai_t *s_nssai)
{
    static char key[32];

    if (!s_nssai)
        return NULL;

    if (s_nssai->sd.v) {  // access the 24-bit value via .v
        snprintf(key, sizeof(key), "%02X-%06X",
                 s_nssai->sst,
                 s_nssai->sd.v);  // <-- use .v
    } else {
        snprintf(key, sizeof(key), "%02X", s_nssai->sst);
    }

    ogs_info("Generated S-NSSAI key: %s", key);
    return key;
}


amf_slice_load_t *amf_slice_load_find(const ogs_s_nssai_t *s_nssai)
{
    ogs_hash_t *hash = amf_self()->slice_load_hash;
    char *key = ogs_strdup(s_nssai_key(s_nssai));
    amf_slice_load_t *slice_load = (amf_slice_load_t *)ogs_hash_get(hash, key, strlen(key));

    if (slice_load) {
        ogs_info("Slice load entry found for key %s: ue_count=%u, threshold=%u",
                 key, slice_load->ue_count, slice_load->threshold);
    } else {
        ogs_info("Slice load entry NOT found for key %s", key);
    }

    ogs_free(key);
    return slice_load;
}

amf_slice_load_t *amf_slice_load_add(const ogs_s_nssai_t *s_nssai, uint32_t threshold)
{
    ogs_hash_t *hash = amf_self()->slice_load_hash;
    char *key = ogs_strdup(s_nssai_key(s_nssai));

    amf_slice_load_t *slice_load = (amf_slice_load_t *)ogs_malloc(sizeof(amf_slice_load_t));
    if(!slice_load) {
        ogs_free(key);
        return NULL;
    }

    slice_load->s_nssai = *s_nssai;
    slice_load->ue_count = 0;
    slice_load->threshold = threshold;

    ogs_hash_set(hash, key, strlen(key), slice_load);
    ogs_info("Added slice load entry: key=%s, threshold=%u", key, threshold);
    return slice_load;
}

void amf_slice_load_remove(const ogs_s_nssai_t *s_nssai)
{
    ogs_hash_t *hash = amf_self()->slice_load_hash;
    char *key = ogs_strdup(s_nssai_key(s_nssai));
    
    amf_slice_load_t *slice_load = (amf_slice_load_t *)ogs_hash_get(hash, key, strlen(key));
    if(slice_load) {
        ogs_info("Removing slice load entry for key %s: ue_count=%u, threshold=%u",
                 key, slice_load->ue_count, slice_load->threshold);
        ogs_free(slice_load); 
    }

    ogs_hash_set(hash, key, strlen(key), NULL);
    ogs_free(key);
}

void amf_slice_load_remove_all(void){
    
}
   

void amf_slice_load_incr(const ogs_nas_s_nssai_ie_t *nas_s_nssai)
{
    if (!nas_s_nssai) return;

    // Map NAS S-NSSAI IE to core S-NSSAI struct
    ogs_s_nssai_t s_nssai = {0};
    s_nssai.sst = nas_s_nssai->sst;
    s_nssai.sd  = nas_s_nssai->sd;

    amf_slice_load_t *slice_load = amf_slice_load_find(&s_nssai);
    if (!slice_load) {
        ogs_info("Slice load entry not found for S-NSSAI %s, cannot increment UE count",
                 s_nssai_key(&s_nssai));
        return;
    }

    __atomic_fetch_add(&slice_load->ue_count, 1, __ATOMIC_RELAXED);

    ogs_info("Slice load UE count incremented for S-NSSAI %s: current ue_count=%u, threshold=%u",
             s_nssai_key(&s_nssai),
             slice_load->ue_count,
             slice_load->threshold);
}

void amf_slice_load_decr(const ogs_nas_s_nssai_ie_t *nas_s_nssai)
{
    if (!nas_s_nssai) return;

    ogs_s_nssai_t s_nssai = {0};
    s_nssai.sst = nas_s_nssai->sst;
    s_nssai.sd  = nas_s_nssai->sd;

    amf_slice_load_t *slice_load = amf_slice_load_find(&s_nssai);
    if (!slice_load) {
        ogs_info("Slice load entry not found for S-NSSAI %s, cannot decrement UE count",
                 s_nssai_key(&s_nssai));
        return;
    }

    if (slice_load->ue_count == 0) {
        ogs_info("Slice load UE count already zero for S-NSSAI %s, cannot decrement",
                 s_nssai_key(&s_nssai));
        return;
    }

    __atomic_fetch_sub(&slice_load->ue_count, 1, __ATOMIC_RELAXED);

    ogs_info("Slice load UE count decremented for S-NSSAI %s: current ue_count=%u, threshold=%u",
             s_nssai_key(&s_nssai),
             slice_load->ue_count,
             slice_load->threshold);
}

uint32_t amf_slice_load_current(const ogs_s_nssai_t *s_nssai)
{
    amf_slice_load_t *slice_load = amf_slice_load_find(s_nssai);

    if(!slice_load) {
        ogs_info("Slice load entry not found for S-NSSAI %s, cannot get current UE count",
                s_nssai_key((ogs_s_nssai_t *)s_nssai));
        return 0;
    }

    return __atomic_load_n(&slice_load->ue_count, __ATOMIC_RELAXED);
}


amf_overload_result_t amf_slice_overload_check(
    const typeof(((amf_ue_t *)0)->requested_nssai) *requested_nssai)
{
    amf_overload_result_t res = {AMF_OVERLOAD_OK};

    if (!requested_nssai || requested_nssai->num_of_s_nssai == 0) {
        ogs_info("No requested NSSAI, skipping slice overload check");
        return res;
    }

    for (int i = 0; i < requested_nssai->num_of_s_nssai; i++) {
        const ogs_nas_s_nssai_ie_t *ie = &requested_nssai->s_nssai[i];
        ogs_s_nssai_t s_nssai = {0};

        s_nssai.sst = ie->sst;
        s_nssai.sd.v = ie->sd.v;  // Just copy, 0 = SD not present

        amf_slice_load_t *slice_load = amf_slice_load_find(&s_nssai);
        if (!slice_load) {
            ogs_info("Slice load entry not found for requested S-NSSAI %s, skipping",
                     s_nssai_key(&s_nssai));
            continue;
        }

        uint32_t cur = __atomic_load_n(&slice_load->ue_count, __ATOMIC_RELAXED);
        if (cur >= slice_load->threshold) {
            ogs_info("Slice overload detected for %s: ue_count=%u >= threshold=%u",
                     s_nssai_key(&s_nssai), cur, slice_load->threshold);

            res.type = AMF_OVERLOAD_REJECT;
            res.backoff_time = 20 + (rand() % 6);
            return res;
        }
    }

    ogs_info("No slice overload detected");
    return res;
}

