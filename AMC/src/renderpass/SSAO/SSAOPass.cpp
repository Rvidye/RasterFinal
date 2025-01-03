#include<common.h>
#include "SSAOPass.h"

void SSAO::create(AMC::RenderContext& context) {

	m_ProgramCompuetSSAO = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\SSAO\\SSAO.comp")});
	glCreateTextures(GL_TEXTURE_2D, 1, &m_textureResult);
	glTextureParameteri(m_textureResult, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_textureResult, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_textureResult, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_textureResult, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(m_textureResult, 1, GL_R8, context.width, context.width);

	context.textureSSAOResult = m_textureResult;
}

void SSAO::execute(AMC::Scene* scene, AMC::RenderContext& context) {
	if (!context.IsSSAO) return;
	//glBindTextureUnit(0, context.textureGBuffer[1]);
	//glBindTextureUnit(1, context.textureGBuffer[4]);
	glBindImageTexture(2, m_textureResult, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8);
	m_ProgramCompuetSSAO->use();
	glUniform1i(m_ProgramCompuetSSAO->getUniformLocation("SampleCount"), SampleCount);
	glUniform1f(m_ProgramCompuetSSAO->getUniformLocation("Radius"), radius);
	glUniform1f(m_ProgramCompuetSSAO->getUniformLocation("Strength"), stength);
	GLuint workGroupSizeX = (context.width + 8 - 1) / 8;
	GLuint workGroupSizeY = (context.height + 8 - 1) / 8;
	glDispatchCompute(workGroupSizeX, workGroupSizeY, 1);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}

const char* SSAO::getName() const
{
	return "SSAO Pass";
}

void SSAO::renderUI()
{
#ifdef _MYDEBUG
	ImGui::SliderInt("Samples", &SampleCount, 1, 20);
	ImGui::SliderFloat("Radius", &radius, 0.0f, 0.5f);
	ImGui::SliderFloat("Stength", &stength, 0.0f, 10.0f);

	if (ImGui::CollapsingHeader("SSAO Texture")) {
		ImGui::Image((void*)(intptr_t)m_textureResult, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	}
#endif
}
