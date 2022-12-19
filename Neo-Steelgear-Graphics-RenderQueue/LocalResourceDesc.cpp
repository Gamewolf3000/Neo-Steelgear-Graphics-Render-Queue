#include "LocalResourceDesc.h"

#include <stdexcept>

LocalResourceDesc::LocalResourceDesc(size_t elementSize,
	size_t nrOfElements, size_t resourceAlignment) :
	elementSize(elementSize), nrOfElements(nrOfElements),
	resourceAlignment(resourceAlignment)
{
	// EMPTY
}

size_t LocalResourceDesc::GetAlignment() const
{
	return resourceAlignment;
}

size_t LocalResourceDesc::GetSize() const
{
	return elementSize * nrOfElements;
}