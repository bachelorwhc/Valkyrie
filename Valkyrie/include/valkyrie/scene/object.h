#ifndef _VALKYRIE_SCENE_OBJECT_H
#define _VALKYRIE_SCENE_OBJECT_H
#include <vector>
#include <set>
#include <memory>
#include "valkyrie/scene/transform.h"
#include "valkyrie/base_unit.h"

namespace ValkyrieComponent {
	class Component;
	using ComponentPtr = std::shared_ptr<Component>;
}

namespace Valkyrie {
	namespace Scene {
		class Object : public Base {
		public:
			Object();
			virtual ~Object();

			virtual void start();
			virtual void update();
			
			Transform transform;
		private:
			//std::set<ValkyrieComponent::ComponentPtr>
		};
	}
}

#endif