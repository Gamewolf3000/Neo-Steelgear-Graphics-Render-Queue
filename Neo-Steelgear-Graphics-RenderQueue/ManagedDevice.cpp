#include "ManagedDevice.h"

inline void ManagedDevice::ThrowIfFailed(HRESULT hr, const std::exception& exception)
{
	if (FAILED(hr))
		throw exception;
}

inline bool ManagedDevice::CheckSupportDXR(ID3D12Device* deviceToCheck,
	D3D12_RAYTRACING_TIER requiredTier)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureData = {};
	HRESULT hr = deviceToCheck->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
		&featureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));

	return SUCCEEDED(hr) && featureData.RaytracingTier >= requiredTier;
}

inline bool ManagedDevice::CheckSupportShaderModel(ID3D12Device* deviceToCheck,
	D3D_SHADER_MODEL requiredModel)
{
	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel;
	shaderModel.HighestShaderModel = requiredModel;
	HRESULT hr = deviceToCheck->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL,
		&shaderModel, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL));

	return SUCCEEDED(hr) && shaderModel.HighestShaderModel >= requiredModel;
}

inline bool ManagedDevice::CheckDevice(ID3D12Device* deviceToCheck,
	const DeviceRequirements& requirements)
{
	return CheckSupportDXR(deviceToCheck, requirements.rtTier) &&
		CheckSupportShaderModel(deviceToCheck, requirements.shaderModel);
}

inline IDXGIAdapter* ManagedDevice::GetAdapter(IDXGIFactory2* factory,
	const DeviceRequirements& requirements)
{
	unsigned int adapterIndex = 0;
	IDXGIAdapter* toReturn = nullptr;

	while (toReturn == nullptr)
	{
		HRESULT hr = factory->EnumAdapters(adapterIndex, &toReturn);

		if (FAILED(hr))
			return nullptr;

		ID3D12Device* temp;
		hr = D3D12CreateDevice(toReturn, requirements.requiredLevel,
			__uuidof(ID3D12Device), reinterpret_cast<void**>(&temp));

		if (FAILED(hr))
			return nullptr;

		if (!CheckDevice(temp, requirements))
		{
			toReturn->Release();
			toReturn = nullptr;
		}

		temp->Release();
		++adapterIndex;
	}

	return toReturn;
}

void ManagedDevice::Initialize(IDXGIFactory2* factory, const DeviceRequirements& requirements)
{
	HRESULT hr = S_OK;
	if (requirements.adapterIndex != std::uint8_t(-1))
	{
		hr = factory->EnumAdapters(requirements.adapterIndex,
			reinterpret_cast<IDXGIAdapter**>(&adapter));
		ThrowIfFailed(hr, std::runtime_error("Could not get desired adapter"));
	}
	else
	{
		adapter = GetAdapter(factory, requirements);
		if (adapter == nullptr)
			throw std::runtime_error("Could not find adapter that satisfies requirements");
	}

	hr = D3D12CreateDevice(adapter, requirements.requiredLevel,
		__uuidof(ID3D12Device), reinterpret_cast<void**>(&device));

	ThrowIfFailed(hr, std::runtime_error("Failed to create device"));
}

ID3D12Device* ManagedDevice::GetDevice()
{
	return device;
}
