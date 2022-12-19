#include "TransientResourceAllocator.h"

#include <stdexcept>

ID3D12Resource* TransientResourceAllocator::AllocateResource(
	const TransientResourceDesc& desc, ID3D12Heap* heap, size_t heapOffset,
	D3D12_RESOURCE_STATES initialState)
{
	ID3D12Resource* toReturn = nullptr;
	HRESULT hr = device->CreatePlacedResource(heap, heapOffset, &desc.GetResourceDesc(),
		initialState, desc.GetOptimalClearValue(), IID_PPV_ARGS(&toReturn));

	if (FAILED(hr))
	{
		throw std::runtime_error("Could not create transient resource");
	}

	return toReturn;
}

void TransientResourceAllocator::AllocateHeapChunk(size_t minimumSize)
{
	HeapChunk chunk = allocator->AllocateChunk(minimumSize,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES);

	memoryChunks.push_back({ chunk, std::vector<AllocatedResource>(), 0 });
}

size_t TransientResourceAllocator::GetAvailableMemoryChunk(size_t allocationSize)
{
	for (size_t i = 0; i < memoryChunks.size(); ++i)
	{
		MemoryChunk& currentChunk = memoryChunks[i];
		size_t chunkSize = currentChunk.heapChunk.endOffset -
			currentChunk.heapChunk.startOffset;

		if ((chunkSize - currentChunk.currentOffset) < allocationSize)
		{
			continue;
		}

		return i;
	}

	AllocateHeapChunk(std::max<size_t>(memoryInfo.expansionSize, allocationSize));
	return memoryChunks.size() - 1;
}

void TransientResourceAllocator::Initialize(ID3D12Device* deviceToUse,
	const TransientAllocatorMemoryInfo& allocatorMemoryInfo,
	HeapAllocatorGPU* allocatorToUse)
{
	device = deviceToUse;
	memoryInfo = allocatorMemoryInfo;
	allocator = allocatorToUse;

	shaderBindableDescriptors.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		deviceToUse, memoryInfo.nrOfStartingSlotsShaderBindable);
	rtvDescriptors.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, deviceToUse,
		memoryInfo.nrOfStartingSlotsRTV);
	dsvDescriptors.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, deviceToUse,
		memoryInfo.nrOfStartingSlotsDSV);
}

void TransientResourceAllocator::Clear()
{
	for (auto& chunk : memoryChunks)
	{
		chunk.resources.clear();
		chunk.currentOffset = 0;
	}

	identifiers.clear();
	shaderBindableDescriptors.Reset();
	rtvDescriptors.Reset();
	dsvDescriptors.Reset();
}

TransientResourceIndex TransientResourceAllocator::CreateTransientResource(
	const TransientResourceDesc& desc, D3D12_RESOURCE_STATES initialState)
{
	size_t requiredSize = desc.CalculateTotalSize(device);
	size_t chunkIndex = GetAvailableMemoryChunk(requiredSize);
	MemoryChunk& chunk = memoryChunks[chunkIndex];
	size_t chunkSize = chunk.heapChunk.endOffset - chunk.heapChunk.startOffset;

	ID3D12Resource* resource = AllocateResource(desc,
		chunk.heapChunk.heap, chunk.currentOffset, initialState);
	std::optional<D3D12_CLEAR_VALUE> optimalClearValue = desc.GetOptimalClearValue() != nullptr ?
		std::optional<D3D12_CLEAR_VALUE>(*desc.GetOptimalClearValue()) : std::nullopt;
	chunk.resources.push_back({ resource, desc.HasRTV(), optimalClearValue });

	size_t newOffset = (((chunk.currentOffset + requiredSize) +
		(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - 1)) &
		~(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - 1));
	chunk.currentOffset = newOffset > chunkSize ? chunkSize : newOffset;

	TransientResourceIdentifier identifier;
	identifier.chunkIndex = chunkIndex;
	identifier.internalIndex = chunk.resources.size() - 1;
	identifiers.push_back(identifier);

	return identifiers.size() - 1;
}

TransientResourceViewIndex TransientResourceAllocator::CreateSRV(
	const TransientResourceIndex& index, std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> desc)
{
	TransientResourceIdentifier identifier = identifiers[index];
	ID3D12Resource* resource = 
		memoryChunks[identifier.chunkIndex].resources[identifier.internalIndex].resource;
	return shaderBindableDescriptors.AllocateSRV(resource, desc.has_value() ? &desc.value() : nullptr);
}

TransientResourceViewIndex TransientResourceAllocator::CreateUAV(
	const TransientResourceIndex& index, std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> desc)
{
	TransientResourceIdentifier identifier = identifiers[index];
	ID3D12Resource* resource =
		memoryChunks[identifier.chunkIndex].resources[identifier.internalIndex].resource;
	return shaderBindableDescriptors.AllocateUAV(resource, desc.has_value() ? &desc.value() : nullptr);
}

TransientResourceViewIndex TransientResourceAllocator::CreateRTV(
	const TransientResourceIndex& index, std::optional<D3D12_RENDER_TARGET_VIEW_DESC> desc)
{
	TransientResourceIdentifier identifier = identifiers[index];
	ID3D12Resource* resource =
		memoryChunks[identifier.chunkIndex].resources[identifier.internalIndex].resource;
	return rtvDescriptors.AllocateRTV(resource, desc.has_value() ? &desc.value() : nullptr);
}

TransientResourceViewIndex TransientResourceAllocator::CreateDSV(
	const TransientResourceIndex& index, std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> desc)
{
	TransientResourceIdentifier identifier = identifiers[index];
	ID3D12Resource* resource =
		memoryChunks[identifier.chunkIndex].resources[identifier.internalIndex].resource;
	return dsvDescriptors.AllocateDSV(resource, desc.has_value() ? &desc.value() : nullptr);
}

size_t TransientResourceAllocator::GetShanderBindableCount() const
{
	return shaderBindableDescriptors.NrOfStoredDescriptors();
}

TransientResourceHandle TransientResourceAllocator::GetTransientResourceHandle(
	const TransientResourceIndex& index) const
{
	TransientResourceIdentifier identifier = identifiers[index];
	ID3D12Resource* resource =
		memoryChunks[identifier.chunkIndex].resources[identifier.internalIndex].resource;
	TransientResourceHandle toReturn = { resource };

	return toReturn;
}

D3D12_CPU_DESCRIPTOR_HANDLE TransientResourceAllocator::GetShaderBindableHandle(
	const TransientResourceViewIndex& index) const
{
	return shaderBindableDescriptors.GetDescriptorHandle(index);
}

D3D12_CPU_DESCRIPTOR_HANDLE TransientResourceAllocator::GetRTV(
	const TransientResourceViewIndex& index) const
{
	return rtvDescriptors.GetDescriptorHandle(index);
}

D3D12_CPU_DESCRIPTOR_HANDLE TransientResourceAllocator::GetDSV(
	const TransientResourceViewIndex& index) const
{
	return dsvDescriptors.GetDescriptorHandle(index);
}

void TransientResourceAllocator::AddInitializationBarriers(std::vector<D3D12_RESOURCE_BARRIER>& toAddTo) const
{
	D3D12_RESOURCE_BARRIER toAdd;
	toAdd.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	toAdd.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	toAdd.Aliasing.pResourceBefore = nullptr;

	for (auto& chunk : memoryChunks)
	{
		for (auto& resource : chunk.resources)
		{
			toAdd.Aliasing.pResourceAfter = resource.resource;
			toAddTo.push_back(toAdd);
		}
	}
}

void TransientResourceAllocator::DiscardRenderTargets(ID3D12GraphicsCommandList* list)
{
	for (auto& identifier : identifiers)
	{
		auto& resource = memoryChunks[identifier.chunkIndex].resources[identifier.internalIndex];
		
		if (resource.hasRTV)
		{
			list->DiscardResource(resource.resource, nullptr);
		}
	}
}

void TransientResourceAllocator::ClearDepthStencils(ID3D12GraphicsCommandList* list)
{
	for (size_t i = 0; i < dsvDescriptors.NrOfStoredDescriptors(); ++i)
	{
		list->ClearDepthStencilView(dsvDescriptors.GetDescriptorHandle(i),
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	}
}
