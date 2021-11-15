#pragma once

#include "object.h"
#include "point.h"

typedef enum { CONNECT_NONE, CONNECT_TOP, CONNECT_BOTTOM, CONNECT_LEFT, CONNECT_RIGHT } CONNECT_POSITION;

struct connectpoint {
	object* n;
	point pt;
	CONNECT_POSITION connect_position;
};