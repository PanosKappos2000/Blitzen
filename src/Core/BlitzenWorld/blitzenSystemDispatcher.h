#pragma once
#include "BlitCL/blitQueue.h"

namespace BlitzenWorld
{
	constexpr uint32_t CE_MAX_QUEUED_REQUESTS = 5;

	enum class ENGINE_SYSTEM_DRIVE_REQUEST : uint32_t
	{
		NO_JOBS,
		PLACEHOLDER,
		EVENT_REGISTER
	};

	enum class JobDriverState : uint8_t
	{
		WORKING,
		HELP_NEEDED,

	};

	class JobDriverQueue
	{
	public:
		JobDriverQueue() = default;

		bool Register(ENGINE_SYSTEM_DRIVE_REQUEST data)
		{
			if (m_count < CE_MAX_QUEUED_REQUESTS)
			{
				m_data[m_count++] = data;
				return true;
			}

			return false;
		}

		ENGINE_SYSTEM_DRIVE_REQUEST Dispatch()
		{
			if (m_count != 0)
			{
				auto res = m_data[m_front];
				if (m_front + 1 == m_count)
				{
					m_front = 0;
					m_count = 0;
				}
				else
				{
					m_front++;
				}

				return res;
			}

			return ENGINE_SYSTEM_DRIVE_REQUEST::NO_JOBS;
		}

		ENGINE_SYSTEM_DRIVE_REQUEST m_data[CE_MAX_QUEUED_REQUESTS];
		uint32_t m_front = 0;
		uint32_t m_count = 0;
		JobDriverState m_state{ JobDriverState::WORKING };
	};

	void StartJobSystem(JobDriverQueue* ptr);

	bool RequestSystemJobDrive(ENGINE_SYSTEM_DRIVE_REQUEST req);
}