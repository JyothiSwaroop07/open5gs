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

/* Function to check if AMF is overloaded (global UE count for now) */
amf_overload_result_t amf_overload_check(ran_ue_t *ran_ue);

#ifdef __cplusplus
}
#endif

#endif /* AMF_OVERLOAD_H */