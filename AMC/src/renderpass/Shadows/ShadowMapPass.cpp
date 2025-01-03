#include<common.h>
#include "ShadowMapPass.h"

void ShadowMapPass::create(AMC::RenderContext& context) {
	//m_programShadowMap = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\shadows\\shadowVS.vert"), RESOURCE_PATH("shaders\\shadows\\shadowGS.geom"), RESOURCE_PATH("shaders\\shadows\\shadowFS.frag") });
	m_programPointShadowMap = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\shadows\\shadowVS.vert"), RESOURCE_PATH("shaders\\shadows\\shadowPointGS.geom"), RESOURCE_PATH("shaders\\shadows\\shadowFS.frag") });
}

void ShadowMapPass::execute(AMC::Scene* scene, AMC::RenderContext& context) {

	if (!context.IsGenerateShadowMaps)
		return;
	AMC::ShadowManager *sm = scene->lightManager->GetShadowManager();
	//sm->renderShadowMaps(m_programShadowMap, scene);
	sm->RenderShadowMaps(m_programPointShadowMap, scene);
	#ifdef _MYDEBUG
	this->sm = sm;
	#endif
}

const char* ShadowMapPass::getName() const
{
	return "Shadowmap Pass";
}

void ShadowMapPass::renderUI()
{
#ifdef _MYDEBUG
	sm->renderUI();
#endif
}
