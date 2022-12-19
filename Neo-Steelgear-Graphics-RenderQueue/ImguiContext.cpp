#include "ImguiContext.h"

#include "Dear ImGui\imgui.h"
#include "Dear ImGui\imgui_impl_win32.h"
#include "Dear ImGui\imgui_impl_dx12.h"

ImguiContext::~ImguiContext()
{
	if (descriptorHeap != nullptr)
	{
		descriptorHeap->Release();

		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
}

void ImguiContext::Initialize(HWND windowHandle, ID3D12Device* deviceToUse)
{
	device = deviceToUse;
	descriptorSize = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.NumDescriptors = MAX_DESCRIPTOR_COUNT;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 0;
	device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(windowHandle);
	ImGui_ImplDX12_Init(device, 1, DXGI_FORMAT_R8G8B8A8_UNORM, descriptorHeap,
		descriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		descriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void ImguiContext::StartImguiFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	currentDescriptorCount = 1;
}

void ImguiContext::FinishImguiFrame(ID3D12GraphicsCommandList* list)
{
	ImGui::Render();
	list->SetDescriptorHeaps(1, &descriptorHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), list);
}

void ImguiContext::AddTextureImage(ID3D12Resource* texture)
{
	auto cpuHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += descriptorSize * currentDescriptorCount;
	device->CreateShaderResourceView(texture, nullptr, cpuHandle);

	auto gpuHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += descriptorSize * currentDescriptorCount;
	D3D12_RESOURCE_DESC resourceDesc = texture->GetDesc();
	ImVec2 dimensions(static_cast<float>(resourceDesc.Width),
		static_cast<float>(resourceDesc.Height));
	ImGui::Image(reinterpret_cast<ImTextureID>(gpuHandle.ptr), dimensions);
}
