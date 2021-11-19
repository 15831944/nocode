#include "node.h"
#include "arrow.h"

object* node::next_arrow(std::list<object*> l, UINT64 generation) {
	for (auto i : l) {
		if (i->isalive(generation) && i->getobjectkind() == OBJECT_ARROW) {
			arrow* a = (arrow*)i;
			if (a->start == this) {
				return a;
			}
		}
	}
	return 0;
}

node* node::next_node(std::list<object*> l, UINT64 generation) {
	arrow* a = (arrow*)next_arrow(l, generation);
	if (a) {
		if (a->end && a->end->isalive(generation)) {
			return a->end;
		}
	}
	return 0;
}
