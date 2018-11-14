//
// pedsim - A microscopic pedestrian simulation system. 
// Copyright (c) 2003 - 2012 by Christian Gloor
//     

#include "tree.h"
#include "config.h"
#include "scene.h"

#include <iostream>

using namespace std;
using namespace Ped;


/// Description: set intial values
/// \date    2012-01-28
Tree::Tree(Scene *pedscene, int pdepth, double px, double py, double pw, double ph) : Ped::Ttree(pedscene, pdepth, px, py, pw, ph) {
	scene = pedscene;
};


/// Destructor: removes the graphics from the screen, too.
/// \date    2012-01-28
Tree::~Tree() {
}

/// \date    2012-02-05
int Tree::cut() {
	return Ttree::cut();	
}

/// New addChildren method. Does basically the same as the base method, but passes the graphicsscene to the children.
/// \date    2012-02-04
void Tree::addChildren() {
	tree1 = new Tree(scene, static_cast<int>(getdepth()) + 1, getx(), gety(), 0.5 * getw(), 0.5 * geth());
	tree2 = new Tree(scene, static_cast<int>(getdepth()) + 1, getx() + 0.5 * getw(), gety(), 0.5 * getw(), 0.5 * geth());
	tree3 = new Tree(scene, static_cast<int>(getdepth()) + 1, getx() + 0.5 * getw(), gety() + 0.5 * geth(), 0.5 * getw(), 0.5 * geth());
	tree4 = new Tree(scene, static_cast<int>(getdepth()) + 1, getx(), gety() + 0.5 * geth(), 0.5 * getw(), 0.5 * geth());
}

