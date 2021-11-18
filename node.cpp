#include "node.h"
#include "arrow.h"

node* node::next_node(std::list<object*> l, UINT64 generation) {
	for (auto i : l) {
		if (i->isalive(generation) && i->getobjectkind() == OBJECT_ARROW) {
			arrow* a = (arrow*)i;
			if (a->start == this) {
				if (a->end && a->end->isalive(generation)) {
					return a->end;
				}
			}
		}
	}
	return 0;
}
