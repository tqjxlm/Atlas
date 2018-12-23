#ifndef NAMESPACE_H
#define NAMESPACE_H

#define MAX_SUBVIEW 2

// Define what to ignore in node traversal
enum TraversalOption
{
	INTERSECT_IGNORE = 0x00000004,
	TRAVERSAL_IGNORE = 0x00000010
};

// Define window splits visibility used in node mask
// and viewer widget
enum WindowVisibility
{
	SHOW_IN_NO_WINDOW  = 0x0fffffff,
	SHOW_IN_ALL_WINDOW = ~SHOW_IN_NO_WINDOW,
	SHOW_IN_WINDOW_1   = 0x10000000,
};

#endif
