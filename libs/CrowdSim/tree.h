#ifndef _tree_h_
#define _tree_h_

#pragma warning(disable:4251)

#include <iostream>
#include <set>
#include <list>

using namespace std;

#include "libpedsim/ped_tree.h"

namespace Ped {
	class Scene;

	class Tree : public Ped::Ttree {
	private:
		Scene *scene;

	public:

		Tree(Scene *pedscene, int depth, double x, double y, double w, double h);
		virtual ~Tree();

		virtual void addChildren();

		int cut();
	};
}

#endif
