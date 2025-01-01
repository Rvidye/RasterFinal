#pragma once

#include <Scene.h>
#include <UBO.h>
#include <MemoryManager.h>

namespace AMC {
	// Store Extra Data that can be accesses by all render passes
	struct RenderContext {
		GLsizei width = 1024, height = 1024;
		GLsizei screenWidth, screenHeight;
		//glm::vec3 GridMin, GridMax;
		//GBuffer
		GLuint textureGBuffer[4]; // albedo, normal, metalroughness, emissive
		//Need Depth to be seperate texture instead of part of array so Vulkan can use it
		Image textureGBufferDepth;

		GBufferDataUBO gBufferData;
		GLuint gBufferUBO;

		SkyBoxUBO SkyBoxData;
		GLuint skyBoxUBO;

		GLuint textureDeferredResult = 0;
		GLuint textureSSAOResult = 0;
		GLuint textureSSRResult = 0;
		GLuint textureVolumetricResult = 0;
		GLuint textureBloomResult = 0;
		GLuint textureTonemapResult = 0;
		GLuint textureAtmosphere = 0;
		GLuint textureVolxelResult = 0;
		GLuint textureVXGIResult = 0;
		GLuint fboPostDeferred = 0; // seems like a hack but fuck it
		GLuint emptyVAO = 0;
	};

	class RenderPass {
		public:
			virtual ~RenderPass() = default;
			virtual void create(RenderContext& context) = 0;
			virtual void execute(Scene* scene, RenderContext &context) = 0;
			virtual void writeDescSet(RenderContext& context) {}
			virtual const char* getName() const = 0;
			virtual void renderUI() = 0;
	};


	class Renderer {

		public:
			static GLsizei width, height;
			static RenderContext context;

			void addPass(RenderPass* pass) {
				passes.push_back(pass);
			}

			void initPasses() {
				for (auto pass : passes) {
					pass->create(context);
				}
			}

			void render(Scene* scene) {
				for (auto pass : passes) {
					pass->execute(scene, context);
				}
			}

			void writeDescSets() {
				for (auto pass : passes) {
					pass->writeDescSet(context);
				}
			}

			static void resetFBO() {
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, width, height);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}

			const std::vector<RenderPass*>& getPasses() const { return passes; }

		private:
			std::vector<RenderPass*> passes;
	};
}
