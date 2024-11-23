#pragma once

#include <common.h>

namespace AMC {

	typedef std::function<void(float)> UpdateCallback;
	typedef std::function<float(float)> EasingFunction;

	struct events_t {
		float start;
		float duration;
		float deltaT; // Interpolation factor between 0.0 and 1.0
		EasingFunction easingFunction;
		UpdateCallback updateFunction;
	};

	class EventManager {

		private:
			float currentTime; // current time of scene or duration of scene
			std::unordered_map<std::string, events_t*> eventList;
			void recalculateTs();

		public:

			EventManager(const std::vector<std::tuple<std::string, float, float, UpdateCallback, EasingFunction>>& events);
			~EventManager();
			void resetEvents();
			float getEventTime(std::string eventName);
			float getCurrentTime();
			void update();
			friend EventManager& operator+=(EventManager& e, float f);
			friend EventManager& operator-=(EventManager& e, float f);
			float& operator[] (std::string eventName);
	};
};

