#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class TestPass : public AMC::RenderPass {

	public:
		void create(AMC::RenderContext& context) override;
		void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
		const char* getName() const override;
		void renderUI() override;
		AMC::ShaderProgram* m_programTexturedDraw;
};
