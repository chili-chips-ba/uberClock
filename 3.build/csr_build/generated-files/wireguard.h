// Generated by PeakRDL-cheader - A free and open-source header generator
//  https://github.com/SystemRDL/PeakRDL-cheader

#ifndef WIREGUARD_H
#define WIREGUARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <assert.h>

// Reg - csr.ip_lookup_engine.table[].allowed_ip[]
#define CSR__IP_LOOKUP_ENGINE__TABLEX__ALLOWED_IPX__ADDRESS_bm 0xffffffff
#define CSR__IP_LOOKUP_ENGINE__TABLEX__ALLOWED_IPX__ADDRESS_bp 0
#define CSR__IP_LOOKUP_ENGINE__TABLEX__ALLOWED_IPX__ADDRESS_bw 32
#define CSR__IP_LOOKUP_ENGINE__TABLEX__ALLOWED_IPX__MASK_bm 0xffffffff00000000
#define CSR__IP_LOOKUP_ENGINE__TABLEX__ALLOWED_IPX__MASK_bp 32
#define CSR__IP_LOOKUP_ENGINE__TABLEX__ALLOWED_IPX__MASK_bw 32
typedef union {
    struct __attribute__ ((__packed__)) {
        uint64_t address :32;
        uint64_t mask :32;
    } f;
    uint64_t w;
} csr__ip_lookup_engine__tablex__allowed_ipx_t;

// Reg - csr.ip_lookup_engine.table[].public_key
#define CSR__IP_LOOKUP_ENGINE__TABLEX__PUBLIC_KEY__KEY_bm 0xffffffff
#define CSR__IP_LOOKUP_ENGINE__TABLEX__PUBLIC_KEY__KEY_bp 0
#define CSR__IP_LOOKUP_ENGINE__TABLEX__PUBLIC_KEY__KEY_bw 32
typedef union {
    struct __attribute__ ((__packed__)) {
        uint32_t key :32;
    } f;
    uint32_t w;
} csr__ip_lookup_engine__tablex__public_key_t;

// Reg - csr.ip_lookup_engine.table[].endpoint
#define CSR__IP_LOOKUP_ENGINE__TABLEX__ENDPOINT__ADDRESS_bm 0xffffffff
#define CSR__IP_LOOKUP_ENGINE__TABLEX__ENDPOINT__ADDRESS_bp 0
#define CSR__IP_LOOKUP_ENGINE__TABLEX__ENDPOINT__ADDRESS_bw 32
#define CSR__IP_LOOKUP_ENGINE__TABLEX__ENDPOINT__PORT_bm 0xffff00000000
#define CSR__IP_LOOKUP_ENGINE__TABLEX__ENDPOINT__PORT_bp 32
#define CSR__IP_LOOKUP_ENGINE__TABLEX__ENDPOINT__PORT_bw 16
typedef union {
    struct __attribute__ ((__packed__)) {
        uint64_t address :32;
        uint64_t port :16;
        uint64_t :16;
    } f;
    uint64_t w;
} csr__ip_lookup_engine__tablex__endpoint_t;

// Regfile - csr.ip_lookup_engine.table[]
typedef struct __attribute__ ((__packed__)) {
    csr__ip_lookup_engine__tablex__allowed_ipx_t allowed_ip[2];
    csr__ip_lookup_engine__tablex__public_key_t public_key;
    uint8_t RESERVED_14_17[0x4];
    csr__ip_lookup_engine__tablex__endpoint_t endpoint;
} csr__ip_lookup_engine__tablex_t;

// Reg - csr.ip_lookup_engine.control
#define CSR__IP_LOOKUP_ENGINE__CONTROL__UPDATE_bm 0x1
#define CSR__IP_LOOKUP_ENGINE__CONTROL__UPDATE_bp 0
#define CSR__IP_LOOKUP_ENGINE__CONTROL__UPDATE_bw 1
#define CSR__IP_LOOKUP_ENGINE__CONTROL__UPDATE_reset 0x0
typedef union {
    struct __attribute__ ((__packed__)) {
        uint32_t update :1;
        uint32_t :31;
    } f;
    uint32_t w;
} csr__ip_lookup_engine__control_t;

// Regfile - csr.ip_lookup_engine
typedef struct __attribute__ ((__packed__)) {
    csr__ip_lookup_engine__tablex_t table[16];
    csr__ip_lookup_engine__control_t control;
} csr__ip_lookup_engine_t;

// Addrmap - csr
typedef struct __attribute__ ((__packed__)) {
    csr__ip_lookup_engine_t ip_lookup_engine;
} csr_t;


static_assert(sizeof(csr_t) == 0x204, "Packing error");

#ifdef __cplusplus
}
#endif

#endif /* WIREGUARD_H */