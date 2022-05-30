/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_STACK
#define LURDS2_STACK

typedef void* Stack;

// A Stack allows push/pop of fixed-size elements
Stack Stack_Create(int32_t elementSize);
void* Stack_Push(Stack stack);
void* Stack_Peek(Stack stack);
void Stack_Pop(Stack stack);
int32_t Stack_Count(Stack stack);
void Stack_Release(Stack stack);

#endif