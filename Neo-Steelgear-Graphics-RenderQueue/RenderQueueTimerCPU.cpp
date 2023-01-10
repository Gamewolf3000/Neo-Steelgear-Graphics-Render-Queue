#include "RenderQueueTimerCPU.h"

double RenderQueueTimerCPU::GetElapsedTime(const RenderQueueTimePoint& startPoint)
{
	if (isActive == false)
	{
		return 0.0;
	}

	RenderQueueTimePoint endPoint = GetCurrentTimePoint();
	std::chrono::duration<double> elapsedTime = (endPoint - startPoint);
	return elapsedTime.count();
}

void RenderQueueTimerCPU::ResetFrameTimes(FrameTimesCPU& toReset)
{
	toReset.totalFrameTime = 0.0;

	toReset.preRenderTime = 0.0;
	toReset.renderTime = 0.0;
	
	toReset.totalPreparationTime = 0.0;
	memset(toReset.batchPreparationTimes.data(), 0,
		sizeof(double) * toReset.batchPreparationTimes.size());
	memset(toReset.jobPreparationTimes.data(), 0,
		sizeof(double) * toReset.jobPreparationTimes.size());

	toReset.setupTime = 0.0;
	toReset.initializationAndUpdateTime = 0.0;
	toReset.discardAndClearTime = 0.0;

	toReset.totalExecutionTime = 0.0;
	memset(toReset.batchExecutionTimes.data(), 0,
		sizeof(double) * toReset.batchExecutionTimes.size());
	memset(toReset.jobExecutionTimes.data(), 0,
		sizeof(double) * toReset.jobExecutionTimes.size());

	toReset.postQueueTime = 0.0;
}

void RenderQueueTimerCPU::SetActive(bool active)
{
	isActive = active;
}

void RenderQueueTimerCPU::Reset()
{
	elapsedGlobalTime += GetElapsedTime(frameStart);
	frameStart = std::chrono::steady_clock::now();
}

void RenderQueueTimerCPU::SetJobInfo(size_t nrOfBatches, size_t nrOfJobs)
{
	if (currentFrameTimes.batchPreparationTimes.size() == nrOfBatches &&
		currentFrameTimes.batchExecutionTimes.size() == nrOfBatches &&
		currentFrameTimes.jobPreparationTimes.size() == nrOfJobs &&
		currentFrameTimes.jobExecutionTimes.size() == nrOfJobs)
	{
		return;
	}

	elapsedGlobalTime = 0.0f;
	elapsedFrames = 0;
	currentFrameTimes.batchPreparationTimes.resize(nrOfBatches);
	currentFrameTimes.jobPreparationTimes.resize(nrOfJobs);
	currentFrameTimes.batchExecutionTimes.resize(nrOfBatches);
	currentFrameTimes.jobExecutionTimes.resize(nrOfJobs);
	ResetFrameTimes(currentFrameTimes);
}

RenderQueueTimePoint RenderQueueTimerCPU::GetCurrentTimePoint()
{
	if (isActive == false)
	{
		return RenderQueueTimePoint();
	}

	return std::chrono::steady_clock::now();
}

RenderQueueTimePoint RenderQueueTimerCPU::MarkPreRender()
{
	currentFrameTimes.preRenderTime += GetElapsedTime(frameStart);

	return GetCurrentTimePoint();
}

void RenderQueueTimerCPU::MarkBatchPreparation(size_t batchIndex,
	const RenderQueueTimePoint& startPoint)
{
	currentFrameTimes.batchPreparationTimes[batchIndex] += GetElapsedTime(startPoint);
}

void RenderQueueTimerCPU::MarkJobPreparation(size_t jobIndex,
	const RenderQueueTimePoint& startPoint)
{
	currentFrameTimes.jobPreparationTimes[jobIndex] += GetElapsedTime(startPoint);
}

void RenderQueueTimerCPU::MarkPreparation(const RenderQueueTimePoint& startPoint)
{
	currentFrameTimes.totalPreparationTime += GetElapsedTime(startPoint);
}

void RenderQueueTimerCPU::MarkSetup(const RenderQueueTimePoint& startPoint)
{
	currentFrameTimes.setupTime += GetElapsedTime(startPoint);
}

void RenderQueueTimerCPU::MarkInitializationAndUpdate(const RenderQueueTimePoint& startPoint)
{
	currentFrameTimes.initializationAndUpdateTime += GetElapsedTime(startPoint);
}

void RenderQueueTimerCPU::MarkDiscardAndClear(const RenderQueueTimePoint& startPoint)
{
	currentFrameTimes.discardAndClearTime += GetElapsedTime(startPoint);
}

void RenderQueueTimerCPU::MarkBatchExecution(size_t batchIndex,
	const RenderQueueTimePoint& startPoint)
{
	currentFrameTimes.batchExecutionTimes[batchIndex] += GetElapsedTime(startPoint);
}

void RenderQueueTimerCPU::MarkJobExecution(size_t jobIndex,
	const RenderQueueTimePoint& startPoint)
{
	currentFrameTimes.jobExecutionTimes[jobIndex] += GetElapsedTime(startPoint);
}

void RenderQueueTimerCPU::MarkExecution(const RenderQueueTimePoint& startPoint)
{
	currentFrameTimes.totalExecutionTime += GetElapsedTime(startPoint);
}

void RenderQueueTimerCPU::MarkPostQueue(const RenderQueueTimePoint& startPoint)
{
	currentFrameTimes.postQueueTime += GetElapsedTime(startPoint);
}

void RenderQueueTimerCPU::MarkImgui(const RenderQueueTimePoint& startPoint)
{
	currentFrameTimes.imguiTime += GetElapsedTime(startPoint);
}

void RenderQueueTimerCPU::FinishFrame(RenderQueueTimePoint renderStartPoint)
{
	currentFrameTimes.renderTime += GetElapsedTime(renderStartPoint);
	double frameTime = GetElapsedTime(frameStart);
	currentFrameTimes.totalFrameTime += frameTime;
	++elapsedFrames;

	if (elapsedGlobalTime >= frequency)
	{
		elapsedGlobalTime -= frequency;
		currentFrameTimes /= elapsedFrames;
		lastCalculatedFrameTimes = currentFrameTimes;
		ResetFrameTimes(currentFrameTimes);
		elapsedFrames = 0;
	}
}

const FrameTimesCPU& RenderQueueTimerCPU::GetFrameTimes()
{
	return lastCalculatedFrameTimes;
}
