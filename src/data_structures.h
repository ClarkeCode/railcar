#ifndef RAILCAR_DATASTRUCTURES
#define RAILCAR_DATASTRUCTURES
#include <stdbool.h>
#include <string.h>

//START: Stack data structure
#define SK_UNPACK(sk_ele_t) *(sk_ele_t*) 
typedef struct _STK{
	size_t size;

	void (*push)(struct _STK*, void*);
	void* (*top)(struct _STK*);
	void* (*pop)(struct _STK*);

	//Size in bytes of elements contained by the stack
	size_t _element_size;
	size_t _capacity;
	void* _data;

} Stack;

Stack* initializeStack(size_t elementSize);
void freeStack(Stack* stack_p);
//END: Stack data structure


//START: Deque data structure
#define DQ_UNPACK(dq_ele_t) *(dq_ele_t*) 
typedef struct _DQ{
	size_t size;

	void (*insert)(struct _DQ*, void* item, size_t index);
	void (*erase)(struct _DQ*, size_t index);

	void (*push_back) (struct _DQ*, void* item);
	void (*push_front)(struct _DQ*, void* item);
	void (*erase_back) (struct _DQ*);
	void (*erase_front)(struct _DQ*);

	void* (*at) (struct _DQ*, size_t index);
	void* (*back) (struct _DQ*);
	void* (*front)(struct _DQ*);
	bool (*in) (struct _DQ*, void* searchItem, bool(*equalityFunc)(void*, void*));
	void* (*find) (struct _DQ*, void* searchItem, bool(*equalityFunc)(void*, void*));

	size_t _element_size;
	size_t _capacity;
	void* _data;
} Deque;

Deque* initializeDeque(size_t elementSize);
void freeDeque(Deque* deque_p);
//END: Deque data structure

#endif