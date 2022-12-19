#include "TransientResourceDesc.h"

#include <stdexcept>

void TransientResourceDesc::InitializeAsBuffer(UINT64 bufferByteSize)
{
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Width = bufferByteSize;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
}

void TransientResourceDesc::InitializeAsTexture2D(UINT64 width, UINT height,
	UINT16 arraySize, UINT16 mipLevels, DXGI_FORMAT format, DXGI_SAMPLE_DESC sampleDesc,
	std::optional<D3D12_CLEAR_VALUE> optimalTextureClearValue)
{
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = arraySize;
	desc.MipLevels = mipLevels;
	desc.Format = format;
	desc.SampleDesc = sampleDesc;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	optimalClearValue = optimalTextureClearValue;
}

void TransientResourceDesc::AddBindFlag(TransientResourceBindFlag bindFlag)
{
	switch (bindFlag)
	{
	case TransientResourceBindFlag::SRV:
		desc.Flags &= ~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		hasSRV = true;
		break;
	case TransientResourceBindFlag::UAV:
		desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		break;
	case TransientResourceBindFlag::RTV:
		desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		break;
	case TransientResourceBindFlag::DSV:
		desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		if (hasSRV == false)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		break;
	default:
		throw std::runtime_error("Unknown bind flag for transient resource detected");
		break;
	}
}

size_t TransientResourceDesc::CalculateTotalSize(ID3D12Device* device) const
{
	auto allocationInfo = device->GetResourceAllocationInfo(0, 1, &desc);
	return allocationInfo.SizeInBytes;
}

bool TransientResourceDesc::HasRTV() const
{
	return desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
}

bool TransientResourceDesc::HasDSV() const
{
	return desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

}

const D3D12_RESOURCE_DESC& TransientResourceDesc::GetResourceDesc() const
{
	return desc;
}

const D3D12_CLEAR_VALUE* TransientResourceDesc::GetOptimalClearValue() const
{
	return optimalClearValue.has_value() ? &optimalClearValue.value() : nullptr;
}