#ifndef COMMANDPOLICY_H
#define COMMANDPOLICY_H

inline int SM_CanDeleteSelection(int disableDelete, int hasSelection)
{
	return !disableDelete && hasSelection;
}

#endif
