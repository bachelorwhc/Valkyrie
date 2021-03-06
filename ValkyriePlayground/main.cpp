#include <iostream>
#include <valkyrie/valkyrie.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan.h>
#include <cassert>
#include <imgui.h>
#include <regex>
#include <queue>
using namespace Valkyrie;

struct ModelViewProjection {
    glm::mat4 world;
    glm::mat4 view;
    glm::mat4 projection;
};

using ModelViewProjections = std::vector<ModelViewProjection>;
using ObjectList = std::vector<unsigned int>;
using VisibleList = std::vector<int>;
std::vector<ObjectList> thread_IDs;
std::vector<ModelViewProjections> thread_MVPs;
std::vector<VisibleList> thread_visiblelists;

void CreateThreadRenderData(std::vector<Vulkan::ThreadCommandPoolPtr>& thread_ptrs, int num_of_threads, int num_of_objects) {
	thread_ptrs.resize(num_of_threads);
	thread_IDs.resize(num_of_threads);
	thread_MVPs.resize(num_of_threads);
	thread_visiblelists.resize(num_of_threads);
	auto& queue = Valkyrie::VulkanManager::getGraphicsQueue();
	auto& factory = ValkyrieFactory::ObjectFactory::instance();
	auto& manager = Valkyrie::ObjectManager::instance();
	std::uniform_real_distribution<float> uniform_distribution(-750.0f, 750.0f);
	auto& random_generator = manager.getRandomGenerator();
	int num_of_objects_per_thread = num_of_objects / num_of_threads;
	for (int i = 0; i < num_of_threads; ++i) {
		thread_ptrs[i] = MAKE_SHARED(Vulkan::ThreadCommandPool)(queue);
		thread_ptrs[i]->initializeSecondaryCommandBuffers(num_of_objects_per_thread);
		thread_MVPs[i].resize(num_of_objects_per_thread);
		thread_visiblelists[i].resize(num_of_objects_per_thread);
		for (int j = 0; j < num_of_objects_per_thread; ++j) {
			auto& object_ptr = factory.createObject();
			thread_IDs[i].push_back(object_ptr->getID());
			object_ptr->transform.setScale(0.1f, 0.1f, 0.1f);
			object_ptr->transform.setTranslate(
				uniform_distribution(random_generator),
				uniform_distribution(random_generator),
				uniform_distribution(random_generator)
			);
		}
	}
}

void ReleaseThreadRenderData(std::vector<Vulkan::ThreadCommandPoolPtr>& thread_ptrs) {
	for (auto& tp : thread_ptrs) {
		Vulkan::DestroyCommandPool(*tp);
	}
}

int CALLBACK WinMain(HINSTANCE instance_handle, HINSTANCE, LPSTR command_line, int command_show) {
	ValkyrieEngine::initializeValkyrieEngine();
	auto& valkyrie = ValkyrieEngine::instance();
	auto& factory = ValkyrieFactory::ObjectFactory::instance();

	std::vector<Vulkan::ThreadCommandPoolPtr> thread_ptrs;
	int num_of_threads = Valkyrie::TaskManager::instance().getNumberOfThreads();
	int num_of_objects = 1024;
	int num_of_objects_per_thread = num_of_objects / num_of_threads;
	CreateThreadRenderData(thread_ptrs, num_of_threads, num_of_objects);

	auto& asset_manager = AssetManager::instance();
	asset_manager.load("duck.lavy");
	asset_manager.load("DuckCM.png");
	auto& mesh_ptr = asset_manager.getMesh("LOD3sp");
	ValkyrieComponent::MeshRenderer mesh_renderer(mesh_ptr);
	Scene::Object duck;
	auto& camera_ptr = factory.createCamera(60, 1024.0f/768.0f, 0.1f, 1000.0f);

    Valkyrie::LightShaderWriter light_shader_writer;
    
    Scene::LightPtr light_ptrs[3] = {
        factory.createLight(Scene::Light::POSITION),
        factory.createLight(Scene::Light::POSITION),
        factory.createLight(Scene::Light::POSITION)
    };
    std::shared_ptr<Scene::PositionLight> position_light_ptrs[3] = {
        std::dynamic_pointer_cast<Scene::PositionLight>(light_ptrs[0]),
        std::dynamic_pointer_cast<Scene::PositionLight>(light_ptrs[1]),
        std::dynamic_pointer_cast<Scene::PositionLight>(light_ptrs[2]),
    };

    position_light_ptrs[0]->setPosition(glm::vec3(187.5, 0.0, 0.0));
    position_light_ptrs[0]->setColor(glm::vec3(1.0, 0.0, 0.0));
    position_light_ptrs[0]->setIntensity(50000.0f);
    position_light_ptrs[1]->setPosition(glm::vec3(0.0, 0.0, 0.0));
    position_light_ptrs[1]->setColor(glm::vec3(0.0, 1.0, 0.0));
    position_light_ptrs[1]->setIntensity(50000.0f);
    position_light_ptrs[2]->setPosition(glm::vec3(-187.5, 0.0, 0.0));
    position_light_ptrs[2]->setColor(glm::vec3(0.0, 0.0, 1.0));
    position_light_ptrs[2]->setIntensity(50000.0f);

    light_shader_writer.addLight(position_light_ptrs[0]->getID());
    light_shader_writer.addLight(position_light_ptrs[1]->getID());
    light_shader_writer.addLight(position_light_ptrs[2]->getID());
    
	auto image_ptr = asset_manager.getImage("DuckCM.png");
	Vulkan::Texture texture(image_ptr);
	VulkanManager::initializeTexture(texture);

	Graphics::Pipeline pipeline;
    {
        PipelineShadersInitializer ps_initializer;
        ps_initializer.initializeShaders("shader.sd", pipeline);
        ps_initializer.initializePool(pipeline);
        ps_initializer.initializeBindings(pipeline);
        ps_initializer.setShaderVariableName(PipelineShadersInitializer::VertexShaderVariableType::POSITION, "in_pos");
        ps_initializer.setShaderVariableName(PipelineShadersInitializer::VertexShaderVariableType::NORMAL, "in_normal");
        ps_initializer.setShaderVariableName(PipelineShadersInitializer::VertexShaderVariableType::UV, "in_uv");
        ps_initializer.initializePipelinePushConstantRanges(pipeline);
        ps_initializer.setPipelineVertexInput(pipeline);
    }
	
	auto& renderer = valkyrie.getRenderer();
	pipeline.initialize(renderer);

    light_shader_writer.updateLights();
    pipeline.descriptorPoolPtr->updateDescriptorSet(light_shader_writer.getPositionLightCountBuffer(), 1, 0);
    pipeline.descriptorPoolPtr->updateDescriptorSet(&light_shader_writer.getPositionLightWrite(), 1, 1, 1);
	pipeline.descriptorPoolPtr->updateDescriptorSet(texture, 1, 2);

	auto ry = 0.0f;

	VkRenderPassBeginInfo render_pass_begin = renderer.getRenderPassBegin();

	auto& task_manager = Valkyrie::TaskManager::instance();
	auto& object_manager = Valkyrie::ObjectManager::instance();

	while (valkyrie.execute()) {
		renderer.prepareFrame();

		int current = renderer.getCurrentBuffer();
		auto& command = renderer.renderCommands[current];
		command.begin();
		render_pass_begin.framebuffer = renderer.getFramebuffer(current);
		vkCmdBeginRenderPass(command.handle, &render_pass_begin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		VkCommandBufferInheritanceInfo inheritance = {};
		inheritance.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritance.renderPass = renderer.getRenderPassHandle();
		inheritance.framebuffer = renderer.getFramebuffer(current);
		ry += 1.0f;
		camera_ptr->transform.getTranslteRef().z = 500.0f + sinf(ry / 100.f) * 500.0f;
		camera_ptr->update();
		auto& camera_properties = camera_ptr->getProperties();

        for (int i = 0; i < 3; ++i) {
            auto sign = i % 2 == 0 ? -1.0f : 1.0f;
            auto& color = position_light_ptrs[i]->getColor();
            color[i] = sign * sinf(ry / 100.0f) * 0.5f + 0.5f;
            position_light_ptrs[i]->setColor(color);
        }
        light_shader_writer.updateLights();

		for (int i = 0; i < num_of_threads; ++i) {
			auto& thread = *thread_ptrs[i];
			auto& objects = thread_IDs[i];
			auto& MVPs = thread_MVPs[i];
			auto& visiblelist = thread_visiblelists[i];
			task_manager.group.run([&]() {
				for (int c = 0; c < num_of_objects_per_thread; ++c) {
					int ID = objects[c];
					auto& object = *object_manager.getObjectPtr(objects[c]);
					
					object.transform.getRotationRef().x = glm::radians<float>(ry);
					object.transform.getRotationRef().z = glm::radians<float>(ry);

                    MVPs[c].world = object.transform.getWorldMatrix();
                    MVPs[c].view = camera_properties.getView();
                    MVPs[c].projection = camera_properties.getPerspective();
					
					visiblelist[c] = camera_properties.frustum.checkPosition(object.transform.getTranslteValue()) ? 1 : 0;
					if (visiblelist[c] == 0)
						continue;

					Vulkan::InheritanceCommandBuffer icb(thread.commands[c], inheritance);
					icb.begin();
					renderer.commandSetViewport(icb);
					renderer.commandSetScissor(icb);
					pipeline.commandBind(icb);

					vkCmdPushConstants(
						icb.handle,
						pipeline.module.layout,
						VK_SHADER_STAGE_VERTEX_BIT,
						0,
						sizeof(ModelViewProjection),
						&MVPs[c]
					);
					mesh_renderer.recordDrawCommand(icb);
					icb.end();
				}
			});
		}
		task_manager.group.wait();

		std::vector<VkCommandBuffer> recorded_commands;
		for(int t = 0; t < num_of_threads; ++t) {
			for (int tvli = 0; tvli < num_of_objects_per_thread; ++tvli) {
				if(thread_visiblelists[t][tvli] > 0)
					recorded_commands.push_back(thread_ptrs[t]->commands[tvli]);
			}
		}

		vkCmdExecuteCommands(
			command.handle,
			recorded_commands.size(),
			recorded_commands.data()
		);

		vkCmdEndRenderPass(command.handle);
		command.end();

		renderer.render();
	}

	ReleaseThreadRenderData(thread_ptrs);
	ValkyrieEngine::closeValkyrieEngine();
	return 0;
}
