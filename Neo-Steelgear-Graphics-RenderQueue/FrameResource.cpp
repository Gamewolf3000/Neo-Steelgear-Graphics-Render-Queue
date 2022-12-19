#include "FrameResource.h"

bool FrameResource::IsWriteState(D3D12_RESOURCE_STATES state) const
{
	switch (state)
	{
	case D3D12_RESOURCE_STATE_RENDER_TARGET:
	case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
	case D3D12_RESOURCE_STATE_DEPTH_WRITE:
	case D3D12_RESOURCE_STATE_STREAM_OUT:
	case D3D12_RESOURCE_STATE_COPY_DEST:
		//case D3D12_RESOURCE_STATE_RESOLVE_DEST: ???
	case D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE:
	case D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE:
	case D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE:
	case D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE:
		return true;
	default:
		return false;
	}
}

FrameResource::FrameResource(const FrameResourceIdentifier& identifier) :
	identifier(identifier)
{
	// EMPTY
}

std::optional<FrameResourceBarrier> FrameResource::UpdateState(D3D12_RESOURCE_STATES newState)
{
	if (currentState == newState && currentState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		// UAV to UAV, needs an UAV barrier
		FrameResourceBarrier toReturn;
		toReturn.InitializeAsUAV(identifier);
		return toReturn;
	}
	else if (currentState != D3D12_RESOURCE_STATE_COMMON &&
		(IsWriteState(currentState) || IsWriteState(newState)) &&
		currentState != newState)
	{
		// States are NOT compatible, transition required

		if (initialTransitionPerformed == false)
		{
			initialState = currentState;
			initialTransitionPerformed = true;
		}

		FrameResourceBarrier toReturn;
		toReturn.InitializeAsTransition(identifier, currentState, newState);

		currentState = newState;

		return toReturn;
	}

	// States are compatible/promotable
	currentState |= newState;
	return std::nullopt;
}

D3D12_RESOURCE_STATES FrameResource::GetInitialState() const
{
	return initialTransitionPerformed == true ? initialState : currentState;
}

D3D12_RESOURCE_STATES FrameResource::GetCurrentState() const
{
	return currentState;
}

bool FrameResource::IsInWriteState() const
{
	return IsWriteState(currentState);
}
