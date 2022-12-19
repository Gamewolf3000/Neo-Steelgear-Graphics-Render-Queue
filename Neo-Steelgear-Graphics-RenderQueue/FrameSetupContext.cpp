#include "FrameSetupContext.h"

#include <stdexcept>

void FrameSetupContext::Reset(size_t nrOfTransientResources)
{
	shaderBindableRequests.clear();
	rtvRequests.clear();
	dsvRequests.clear();

	if (transientResourceDescs.size() < nrOfTransientResources)
	{
		transientResourceDescs.resize(nrOfTransientResources);
	}

	localResourceDescs.clear();
	totalLocalMemoryNeeded = 0;
}

void FrameSetupContext::SetTransientResourceDesc(
	const TransientResourceIndex& index, const TransientResourceDesc& desc)
{
	transientResourceDescs[index] = desc;
}

const TransientResourceDesc& FrameSetupContext::GetTransientResourceDesc(
	const TransientResourceIndex& index)
{
	return transientResourceDescs[index];
}

LocalResourceIndex FrameSetupContext::CreateLocalResource(const LocalResourceDesc& desc)
{
	size_t startOffset = ((totalLocalMemoryNeeded + 
		(desc.GetAlignment() - 1)) & ~(desc.GetAlignment() - 1));
	totalLocalMemoryNeeded = startOffset + desc.GetSize();
	localResourceDescs.push_back(desc);
	return localResourceDescs.size() - 1;
}

ViewIdentifier FrameSetupContext::RequestTransientShaderBindable(
	const DescriptorRequest<ShaderBindableDescriptorDesc>& request)
{
	shaderBindableRequests.push_back(request);

	auto& desc = transientResourceDescs[request.index];

	switch (request.info.type)
	{
	case ShaderBindableDescriptorType::SHADER_RESOURCE:
		desc.AddBindFlag(TransientResourceBindFlag::SRV);
		break;
	case ShaderBindableDescriptorType::UNORDERED_ACCESS:
		desc.AddBindFlag(TransientResourceBindFlag::UAV);
		break;
	default:
		throw std::runtime_error("Unknown shader bindable descriptor type for transient resource");
		break;
	}

	ViewIdentifier toReturn;
	toReturn.type = FrameViewType::SHADER_BINDABLE;
	toReturn.internalIndex = shaderBindableRequests.size() - 1;

	return toReturn;
}

ViewIdentifier FrameSetupContext::RequestTransientRTV(
	const DescriptorRequest<std::optional<D3D12_RENDER_TARGET_VIEW_DESC>>& request)
{
	rtvRequests.push_back(request);

	auto& desc = transientResourceDescs[request.index];
	desc.AddBindFlag(TransientResourceBindFlag::RTV);

	ViewIdentifier toReturn;
	toReturn.type = FrameViewType::RTV;
	toReturn.internalIndex = rtvRequests.size() - 1;

	return toReturn;
}

ViewIdentifier FrameSetupContext::RequestTransientDSV(
	const DescriptorRequest<std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC>>& request)
{
	dsvRequests.push_back(request);

	auto& desc = transientResourceDescs[request.index];
	desc.AddBindFlag(TransientResourceBindFlag::DSV);

	ViewIdentifier toReturn;
	toReturn.type = FrameViewType::DSV;
	toReturn.internalIndex = dsvRequests.size() - 1;

	return toReturn;
}