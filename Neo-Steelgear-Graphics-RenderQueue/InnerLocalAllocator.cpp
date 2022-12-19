#include "InnerLocalAllocator.h"

#include <stdexcept>

void InnerLocalAllocator::AllocateResource()
{
	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Width = currentSize;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	
	HRESULT hr = device->CreatePlacedResource(heapChunk.heap, 
		heapChunk.startOffset, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&resource));
	
	if (FAILED(hr))
	{
		auto error = "Failed to allocate resource in heap in inner local resource allocator";
		throw std::runtime_error(error);
	}

	D3D12_RANGE nothing = { 0, 0 }; // We only write, we do not read
	hr = resource->Map(0, &nothing, reinterpret_cast<void**>(&mappedPtr));

	if (FAILED(hr))
	{
		throw std::runtime_error("Could not map inner local resource buffer");
	}
}

void InnerLocalAllocator::AllocateHeapChunk(size_t minimumSize)
{
	if (resource != nullptr)
	{
		resource->Release();
		allocator->DeallocateChunk(heapChunk);
	}

	heapChunk = allocator->AllocateChunk(minimumSize,
		D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
	currentSize = heapChunk.endOffset - heapChunk.startOffset;
	AllocateResource();
}

InnerLocalAllocator::~InnerLocalAllocator()
{
	if (resource != nullptr)
	{
		resource->Release();
		allocator->DeallocateChunk(heapChunk);
	}
}

void InnerLocalAllocator::Initialize(ID3D12Device* deviceToUse,
	const LocalAllocatorMemoryInfo& allocatorMemoryInfo,
	HeapAllocatorGPU* allocatorToUse)
{
	device = deviceToUse;
	memoryInfo = allocatorMemoryInfo;
	allocator = allocatorToUse;
	AllocateHeapChunk(allocatorMemoryInfo.initialSize);
}

void InnerLocalAllocator::Reset()
{
	buffers.clear();
	currentOffset = 0;
}

void InnerLocalAllocator::SetMinimumFrameDataSize(size_t minimumSizeNeeded)
{
	if (currentSize < minimumSizeNeeded)
	{
		size_t newSize = currentSize + memoryInfo.expansionSize >= minimumSizeNeeded ?
			currentSize + memoryInfo.expansionSize : minimumSizeNeeded;
		AllocateHeapChunk(newSize);
	}
}

LocalResourceIndex InnerLocalAllocator::AllocateBuffer(const LocalResourceDesc& desc)
{
	size_t startOffset = ((currentOffset + (desc.GetAlignment() - 1)) & ~(desc.GetAlignment() - 1));
	currentOffset = startOffset + desc.GetSize();
	BufferEntry toAdd = { startOffset, desc.GetSize() };
	buffers.push_back(toAdd);

	return buffers.size() - 1;
}

LocalResourceHandle InnerLocalAllocator::GetHandle(const LocalResourceIndex& index) const
{
	LocalResourceHandle toReturn;
	toReturn.resource = resource;
	toReturn.offset = buffers[index].offset;
	toReturn.size = buffers[index].size;

	return toReturn;
}

size_t InnerLocalAllocator::GetCurrentSize() const
{
	return currentSize;
}

D3D12_RESOURCE_BARRIER InnerLocalAllocator::GetInitializationBarrier()
{
	D3D12_RESOURCE_BARRIER toReturn;
	toReturn.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	toReturn.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	toReturn.Aliasing.pResourceBefore = nullptr;
	toReturn.Aliasing.pResourceAfter = resource;

	return toReturn;
}

void InnerLocalAllocator::UpdateData(void* dataPtr, size_t dataSize)
{
	memcpy(mappedPtr, dataPtr, dataSize);
}