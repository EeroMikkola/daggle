#pragma once

#include "stdint.h"

#include <daggle/daggle.h>

// data_entry {
//     type_stoff: u64 (reuse typename strings)
//     size: u64
//     bytes: byte[size]
// }

typedef struct data_entry_1_s {
	uint64_t type_stoff;
	uint64_t size;
	unsigned char bytes[/*size*/];
} data_entry_1_t; // size = 2*u64+size

// strings like: "first\0second\0third\0", access by Nth character

// version: u64
// num_nodes: u64
// num_ports: u64
// strings_len: u64
// datas_len: u64
// nodes: node_entry_1_s[num_nodes]
// ports: port_entry_1_s[num_ports]
// strings: char[strings_len]
// datas: byte[datas_len], (bytes encode data_entry_1_t[])

// Use versioned structs for backwards compatibility.

typedef struct node_entry_1_s {
	uint64_t name_stoff;
	uint64_t type_stoff;
	uint64_t num_ports;
	uint64_t first_port_ptidx;
} node_entry_1_t;

typedef enum port_variant_1_e {
	INPUT,
	OUTPUT,
	PARAMETER,
} port_variant_1_t;

typedef enum input_variant_1_e {
	IMMUTABLE_REFERENCE, 
	IMMUTABLE_COPY,
	MUTABLE_REFERENCE,
	MUTABLE_COPY
} input_variant_1_t;

typedef struct port_entry_1_s {
	uint64_t name_stoff;
	uint64_t edge_ptidx; // max if unset
	uint64_t data_dtoff; // max if unset
	port_variant_1_t port_variant;
	union {
		input_variant_1_t input;
	} port_specific;
} port_entry_1_t;
