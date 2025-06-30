#pragma once

#include "stdint.h"

#include <daggle/daggle.h>

// port_flags {
//     input
//     output
//     param
//     recycle
//     has_data
//     linked
// }

// data_entry {
//     type_stoff: u64 (reuse typename strings)
//     size: u64
//     bytes: byte[size]
// }

// strings like: "first\0second\0third\0", access by Nth character

// version: u64
// num_nodes: u64
// num_ports: u64
// nodes: node_entry_1_s[num_nodes]
// ports: port_entry_1_s[num_ports]
// strings_len: u64
// strings: char[strings_len]
// datas_len: u64
// datas: byte[datas_len], (bytes encode data_entry[])

typedef struct node_entry_1_s {
	uint64_t name_stoff;
	uint64_t type_stoff;
	uint64_t num_ports;
	uint64_t first_port_ptidx;
} node_entry_1_t;

typedef struct port_entry_1_s {
	uint64_t name_stoff;
	uint64_t edge_ptidx; // Set if flags has linked, else 0
	uint64_t data_dtoff; // Set if flags has has_data, else 0
	uint8_t flags;
} port_entry_1_t;
