#pragma once

typedef struct void_closure_s {
	void (*function)(void* ctx);
	void (*dispose)(void* ctx);
	void* context;
} void_closure_t;

void
void_closure_call(void_closure_t* closure);

void
void_closure_dispose(void_closure_t* closure);
