#include "blitTimeManager.h"
#include "Platform/blitPlatform.h"

namespace BlitzenCore
{
    inline WorldTimeManager* GSClock{ nullptr };

    WorldTimeManager::WorldTimeManager() :
        mElapsedTime{ 0.0 }, mPreviousTime{ 0.0 }, mDeltaTime{0.0}
	{
        BlitzenPlatform::PlatfrormSetupClock(this);
        GSClock = this;
    }

    void WorldTimeManager::Startup()
    {
        mStartTime = BlitzenPlatform::PlatformGetAbsoluteTime(mClockFrequency);
    }

    void WorldTimeManager::Update()
    {
        mElapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime(mClockFrequency) - mStartTime;
        mDeltaTime = mElapsedTime - mPreviousTime <= GCMaxTimeStep ? mElapsedTime - mPreviousTime : GCMaxTimeStep;
        mPreviousTime = mElapsedTime;
    }

    double WorldTimeManager::GetAbsoluteTime()
    {
        return BlitzenPlatform::PlatformGetAbsoluteTime(mClockFrequency);
    }

    void BlitPerformanceCounter::Generate(WorldTimeManager* pClock)
    {
        mClockFrequency = pClock->mClockFrequency;
    }

    void BlitPerformanceCounter::GenerateInner()
    {
        mClockFrequency = GSClock->mClockFrequency;
    }

    double BlitPerformanceCounter::Startup()
    {
        mStartTime = BlitzenPlatform::PlatformGetAbsoluteTime(mClockFrequency);
        return mStartTime;
    }

    double BlitPerformanceCounter::End()
    {
        mEndTime = BlitzenPlatform::PlatformGetAbsoluteTime(mClockFrequency) - mStartTime;
        return mEndTime;
    }

    void BlitPerformanceCounter::Reset()
    {
        mStartTime = 0.0;
        mEndTime = 0.0;
    }
}