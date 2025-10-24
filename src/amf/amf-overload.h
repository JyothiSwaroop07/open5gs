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
} amf_slice_load_t;

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
void amf_slice_load_remove_all(void);

// Increment, decrement, get current UE count for a slice
void amf_slice_load_incr(const ogs_s_nssai_t *s_nssai);
void amf_slice_load_decr(const ogs_s_nssai_t *s_nssai);
uint32_t amf_slice_load_current(const ogs_s_nssai_t *s_nssai);

// Check overload based on slice load
amf_overload_result_t amf_slice_overload_check(const typeof(((amf_ue_t *)0)->requested_nssai) *requested_nssai);


#ifdef __cplusplus
}
#endif

#endif /* AMF_OVERLOAD_H */