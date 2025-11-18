#ifndef AMF_OVERLOAD_H
#define AMF_OVERLOAD_H

#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Overload check result types */
typedef enum {
    AMF_OVERLOAD_OK,
    AMF_OVERLOAD_REJECT,
    AMF_OVERLOAD_DROP
} amf_overload_type_t;

/* Overload check result structure */
typedef struct {
    amf_overload_type_t type;
    uint32_t backoff_time; // in seconds, valid if type is AMF_OVERLOAD_REJECT
} amf_overload_result_t;

typedef struct amf_slice_load_s {
    ogs_s_nssai_t s_nssai;
    uint32_t ue_count;
    uint32_t threshold;
    uint32_t reg_req_count_per_slice;
    uint32_t rps_per_slice;
    ogs_hash_t *dnn_hash;
} amf_slice_load_t;

typedef struct amf_dnn_load_s {
    char dnn[OGS_MAX_DNN_LEN];
    uint32_t ue_count;
    uint32_t threshold;
    uint32_t reg_req_count_per_dnn;
    uint32_t rps_per_dnn;
} amf_dnn_load_t;

/* Function to check if AMF is overloaded (global UE count for now) */
amf_overload_result_t amf_overload_check(ran_ue_t *ran_ue);

uint8_t encode_t3346(uint32_t seconds);

//Slice based overload management functions

// Generate S-NSSAI key string
char *s_nssai_key(const ogs_s_nssai_t *s_nssai);

// Find, add, remove slice load entry
amf_slice_load_t *amf_slice_load_find(const ogs_s_nssai_t *s_nssai);
amf_slice_load_t *amf_slice_load_add(const ogs_s_nssai_t *s_nssai, uint32_t threshold);
void amf_slice_load_remove(const ogs_s_nssai_t *s_nssai);
void amf_slice_load_hash_cleanup(void);

// Increment, decrement, get current UE count for a slice
void amf_slice_load_incr(const ogs_nas_s_nssai_ie_t *nas_s_nssai);
void amf_slice_load_decr(const ogs_nas_s_nssai_ie_t *nas_s_nssai);
uint32_t amf_slice_load_current(const ogs_s_nssai_t *s_nssai);

// Check overload based on slice load
amf_overload_result_t amf_slice_overload_check(const typeof(((amf_ue_t *)0)->requested_nssai) *requested_nssai);

// RPS timer callback
void amf_overload_rps_timer_cb(void *data);

// Slice RPS functions
void amf_slice_rps_incr(ogs_nas_s_nssai_ie_t *nas_s_nssai);
void amf_slice_rps_reset(void);

// DNN overload check function
amf_overload_result_t amf_dnn_overload_check(
    const ogs_s_nssai_t *s_nssai, const char *dnn);
amf_dnn_load_t *amf_dnn_load_add(amf_slice_load_t *slice_load, const char *dnn, uint32_t threshold);
amf_dnn_load_t *amf_dnn_load_find(amf_slice_load_t *slice_load, const char *dnn);

bool amf_n2_is_overloaded(void);

#ifdef __cplusplus
}
#endif

#endif /* AMF_OVERLOAD_H */