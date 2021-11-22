#include "node_if.h"
#include "arrow.h"

object* node_if::next_arrow(std::list<object*> l, UINT64 generation)
{
	for (auto i : l) {
		if (i->isalive(generation) && i->getobjectkind() == OBJECT_ARROW) {
			arrow* a = (arrow*)i;
			if (a->start == this) {
				return a;
			}
		}
	}
	return nullptr;
}
