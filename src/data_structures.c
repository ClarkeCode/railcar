#include "data_structures.h"

#define RC_DATASTRUCTURE_DEFAULT_CAPACITY 16
typedef unsigned char BYTE_;

//START: Stack data structure
void stackPush(Stack* stack, void* newElement) {
	
	Node* wrapper = (void*) calloc(1, sizeof(Node));
	wrapper->_data = newElement;
	wrapper->parent = stack->_stack;
	stack->_stack = wrapper;

	stack->size++;
}

void* stackTop(Stack* stack) {
	if (stack->size == 0) return NULL;
	return stack->_stack->_data;
}

void* stackPop(Stack* stack) {
	Node* oldTop = stack->_stack;
	void* content = oldTop->_data;
	stack->size--;
	if (oldTop) free(oldTop);
	return content;
}

//Public
Stack* initializeStack(size_t elementSize) {
	Stack* stack = (void*)calloc(1, sizeof(Stack));
	// stack->_stack = NULL;
	stack->_element_size = elementSize;

	stack->push = &stackPush;
	stack->top  = &stackTop;
	stack->pop  = &stackPop;

	return stack;
}
void freeStack(Stack* stack_p) {
	// if (stack_p && stack_p->_data) free(stack_p->_data);
	if (stack_p)                   free(stack_p);
}
//END: Stack data structure







//START: Deque data structure
void _dequeConditionalResize(Deque* deque) {
	if (deque->size == deque->_capacity) {
		deque->_capacity *= 2;
		deque->_data = (void*)realloc(deque->_data, deque->_capacity);
	}
}

void* dequeAt(Deque* deque, size_t index) {
	if (index >= deque->size) return NULL;
	return (BYTE_*)deque->_data + (index * deque->_element_size);
}

void _dequeShiftLeft(Deque* deque, size_t startIndex) {
	for (size_t x = startIndex; x < deque->size-1; x++) {
		memcpy(dequeAt(deque, x), dequeAt(deque, x+1), deque->_element_size);
	}
}
//Note: Assumes the deque has appropriate capacity to shift right
void _dequeShiftRight(Deque* deque, size_t startIndex) {
	for (size_t x = deque->size-1; x > startIndex; x--) {
		memcpy(dequeAt(deque, x), dequeAt(deque, x-1), deque->_element_size);
	}
}

void dequeInsert(Deque* deque, void* newItem, size_t index) {
	deque->size++;
	_dequeConditionalResize(deque);
	if (index < deque->size-1) {
		//shift elements right
		_dequeShiftRight(deque, index);
	}

	memcpy(dequeAt(deque, index), newItem, deque->_element_size);
}

void dequeErase(Deque* deque, size_t index) {
	_dequeShiftLeft(deque, index);
	deque->size--;
}

void dequeEraseFront(Deque* deque) { dequeErase(deque, 0); }
void dequeEraseBack(Deque* deque)  { dequeErase(deque, deque->size-1); }
void dequePushFront(Deque* deque, void* newItem) { dequeInsert(deque, newItem, 0); }
void dequePushBack(Deque* deque, void* newItem)  { dequeInsert(deque, newItem, deque->size); }

void* dequeBack(Deque* deque) { return dequeAt(deque, deque->size-1); }
void* dequeFront(Deque* deque) { return dequeAt(deque, 0); }

bool dequeIn(Deque* deque, void* searchItem, bool(*equalityFunc)(void*, void*)) {
	for (size_t x = 0; x < deque->size; x++) {
		if (equalityFunc(searchItem, dequeAt(deque, x))) {
			return true;
		}
	}
	return false;
}

void* dequeFind(Deque* deque, void* searchItem, bool(*equalityFunc)(void*, void*)) {
	if (!dequeIn(deque, searchItem, equalityFunc)) return NULL;
	for (size_t x = 0; x < deque->size; x++) {
		if (equalityFunc(searchItem, dequeAt(deque, x))) {
			return dequeAt(deque, x);
		}
	}
	return NULL;
}

Deque* initializeDeque(size_t elementSize) {
	Deque* deque = (void*)calloc(1, sizeof(Deque));
	deque->_capacity = RC_DATASTRUCTURE_DEFAULT_CAPACITY;
	deque->_element_size = elementSize;
	deque->_data = (void*)calloc(RC_DATASTRUCTURE_DEFAULT_CAPACITY, elementSize);

	//methods
	deque->insert = &dequeInsert;
	deque->erase = &dequeErase;

	deque->push_back   = &dequePushBack;
	deque->push_front  = &dequePushFront;
	deque->erase_back  = &dequeEraseBack;
	deque->erase_front = &dequeEraseFront;

	deque->at = &dequeAt;
	deque->back = &dequeBack;
	deque->front = &dequeFront;
	deque->in = &dequeIn;
	deque->find = &dequeFind;

	return deque;
}
void freeDeque(Deque* deque) {
	if (deque && deque->_data) free(deque->_data);
	if (deque)                 free(deque);
}
//END: Deque data structure
