#include<common.h>
#include "SSR.h"

void SSR::create(AMC::RenderContext& context) {
	m_ProgramSSR = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\SSR\\SSR.comp")});
	m_ProgramMergeTextures = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\mergetextures\\merge.comp") });
	glCreateTextures(GL_TEXTURE_2D, 1, &textureSSR);
	glTextureParameteri(textureSSR, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(textureSSR, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(textureSSR, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(textureSSR, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(textureSSR, 1, GL_RGBA16F, context.width, context.width);
	context.textureSSRResult = textureSSR;
}

void SSR::execute(const AMC::Scene* scene, AMC::RenderContext& context) {

	glBindTextureUnit(0, context.textureGBuffer[0]); // albedo
	glBindTextureUnit(1, context.textureGBuffer[1]); // normal
	glBindTextureUnit(2, context.textureGBuffer[2]); // metal-roughness
	glBindTextureUnit(3, context.textureGBuffer[4]); // depth
	glBindTextureUnit(4, context.textureDeferredResult); // sampler src
	glBindTextureUnit(5, context.textureAtmosphere); // skyAlbedo
	glBindImageTexture(0, textureSSR, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F); // ImgResult
	m_ProgramSSR->use();
	glUniform1i(m_ProgramSSR->getUniformLocation("SampleCount"), 30);
	glUniform1i(m_ProgramSSR->getUniformLocation("BinarySearchCount"), 8);
	glUniform1f(m_ProgramSSR->getUniformLocation("MaxDist"), 50.0f);
	GLuint workGroupSizeX = (context.width + 8 - 1) / 8;
	GLuint workGroupSizeY = (context.height + 8 - 1) / 8;
	glDispatchCompute(workGroupSizeX, workGroupSizeY, 1);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

	// Perform merge texture just in case
	glBindImageTexture(0, context.textureDeferredResult, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F); // ImgResult
	glBindTextureUnit(0, context.textureDeferredResult); // first
	glBindTextureUnit(1, textureSSR); // second
	m_ProgramMergeTextures->use();
	workGroupSizeX = (context.width + 8 - 1) / 8;
	workGroupSizeY = (context.height + 8 - 1) / 8;
	glDispatchCompute(workGroupSizeX, workGroupSizeY, 1);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}