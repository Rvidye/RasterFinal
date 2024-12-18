#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class TestPass : public AMC::RenderPass {

	public:
		void create(AMC::RenderContext& context) override;
		void execute(const AMC::Scene* scene, AMC::RenderContext& context) override;
		AMC::ShaderProgram* m_programTexturedDraw;
};
