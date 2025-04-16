#pragma once

#define list_push(list, x) do { (list)->elems[(list)->size++] = (x); } while (0)
#define list_pop(list) do { (list)->size--; } while (0)
#define list_clear(list) do { (list)->size = 0; } while (0)
