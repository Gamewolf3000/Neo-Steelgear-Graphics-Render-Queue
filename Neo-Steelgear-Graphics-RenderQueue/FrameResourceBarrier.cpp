#include "FrameResourceBarrier.h"

void FrameResourceBarrier::InitializeAsTransition(
	const FrameResourceIdentifier& identifier,
	D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
	type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	data.transition.identifier = identifier;
	data.transition.stateBefore = stateBefore;
	data.transition.stateAfter = stateAfter;
}

void FrameResourceBarrier::InitializeAsAliasing(
	const FrameResourceIdentifier& identifierBefore,
	const FrameResourceIdentifier& identifierAfter)
{
	type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	data.aliasing.identifierBefore = identifierBefore;
	data.aliasing.identifierAfter = identifierAfter;
}

void FrameResourceBarrier::InitializeAsUAV(
	const FrameResourceIdentifier& identifier)
{
	type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	data.uav.identifier = identifier;
}

void FrameResourceBarrier::MergeTransitionAfterState(
	D3D12_RESOURCE_STATES stateToMerge)
{
	data.transition.stateAfter |= stateToMerge;
}