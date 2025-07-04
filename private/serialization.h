#pragma once

#include "stdint.h"

#include <daggle/daggle.h>

// Use versioned structs for backwards compatibility.

typedef struct graph_1_s {
	uint64_t version;
	uint64_t num_nodes;
	uint64_t num_ports;
	uint64_t text_len;
	uint64_t data_len;
	unsigned char bytes[];
} graph_1_t;

// graph_1_t bytes field encodes:
// - nodes: node_entry_1_s[num_nodes]
// - ports: port_entry_1_s[num_ports]
// - strings: char[text_len], (stored like "first\0second\0")
// - datas: unsigned char[data_len], (bytes encode data_entry_1_t[])

typedef struct data_entry_1_s {
	uint64_t type_stoff;
	uint64_t size;
	unsigned char bytes[/*size*/];
} data_entry_1_t;

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

typedef enum input_behavior_1_e {
	REFERENCE,
	CLONE,
	ACQUIRE
} input_behavior_1_t;

typedef struct port_entry_1_s {
	uint64_t name_stoff;
	uint64_t edge_ptidx; // max if unset
	uint64_t data_dtoff; // max if unset
	port_variant_1_t port_variant;
	union {
		input_behavior_1_t input;
	} port_specific;
} port_entry_1_t;
