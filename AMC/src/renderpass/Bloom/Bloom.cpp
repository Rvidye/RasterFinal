#include<common.h>
#include "Bloom.h"

int AMC::GetMaxMipmapLevel(int width, int height, int depth) {
    int largestDimension = std::max({ width, height, depth });
    return static_cast<int>(std::log2(largestDimension)) + 1;
}

glm::ivec3 AMC::GetMipmapLevelSize(int width, int height, int depth, int level) {
    // Divide each dimension by 2^level
    glm::ivec3 size = glm::ivec3(width, height, depth) / (1 << level);
    // Ensure each dimension is at least 1
    return glm::max(size, glm::ivec3(1));
}

void Bloom::create(AMC::RenderContext& context) {

	minusLods = 3;
	threshold = 1.0f;
	maxColor = 2.8f;

    AMC::bloom_threshold = threshold;
    AMC::bloom_maxcolor = maxColor;

	m_ProgramBloom = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\Bloom\\Bloom.comp")});

	texWidth = context.width / 2;
	texHeight = context.height / 2;

	levels = std::max(AMC::GetMaxMipmapLevel(texWidth,texHeight,1) - minusLods, 2);

	glCreateTextures(GL_TEXTURE_2D, 1, &textureDownsample);
	glTextureParameteri(textureDownsample, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTextureParameteri(textureDownsample, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(textureDownsample, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(textureDownsample, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(textureDownsample, levels, GL_RGBA16F, texWidth, texHeight);

	glCreateTextures(GL_TEXTURE_2D, 1, &textureUpsample);
	glTextureParameteri(textureUpsample, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTextureParameteri(textureUpsample, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(textureUpsample, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(textureUpsample, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(textureUpsample, levels - 1, GL_RGBA16F, texWidth, texHeight);

    context.textureBloomResult = textureUpsample;
}

void Bloom::execute(AMC::Scene* scene, AMC::RenderContext& context) {

    if (!context.IsBloom) {
        context.textureBloomResult = NULL;
        return;
    }
    else
    {
        context.textureBloomResult = textureUpsample;
    }

    m_ProgramBloom->use();
    int currentWriteLod = 0;
    glUniform1f(2, AMC::bloom_threshold);
    glUniform1f(3, AMC::bloom_maxcolor);
    // Downsampling stage
    {
        glBindImageTexture(0, textureDownsample, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindTextureUnit(0, context.textureGBuffer[3]);
        glUniform1i(0, currentWriteLod);
        glUniform1i(1, 0); // 0 for Downsample stage
        glm::ivec3 mipLevelSize = AMC::GetMipmapLevelSize(texWidth, texHeight, 1, currentWriteLod);
        glDispatchCompute((mipLevelSize.x + 8 - 1) / 8, (mipLevelSize.y + 8 - 1) / 8, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
        currentWriteLod++;
    }

    glBindTextureUnit(0, textureDownsample);
    // Additional downsample levels
    for (; currentWriteLod < levels; currentWriteLod++) {
        glBindImageTexture(0, textureDownsample, currentWriteLod, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glUniform1i(0, currentWriteLod - 1);
        glm::ivec3 mipLevelSize = AMC::GetMipmapLevelSize(texWidth, texHeight, 1, currentWriteLod);
        glDispatchCompute((mipLevelSize.x + 8 - 1) / 8, (mipLevelSize.y + 8 - 1) / 8, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    // Upsampling stage
    currentWriteLod = levels - 2;
    {
        glBindTextureUnit(1, textureDownsample);
        glBindImageTexture(0, textureUpsample, currentWriteLod, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glUniform1i(0, currentWriteLod + 1);
        glUniform1i(1, 1); // 1 for Upsample stage
        glm::ivec3 mipLevelSize = AMC::GetMipmapLevelSize(texWidth, texHeight, 1, currentWriteLod);
        glDispatchCompute((mipLevelSize.x + 8 - 1) / 8, (mipLevelSize.y + 8 - 1) / 8, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
        currentWriteLod--;
    }

    glBindTextureUnit(1, textureUpsample);
    for (; currentWriteLod >= 0; currentWriteLod--) {
        glBindImageTexture(0, textureUpsample, currentWriteLod, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glUniform1i(0, currentWriteLod + 1);
        glUniform1i(1, 1);
        glm::ivec3 mipLevelSize = AMC::GetMipmapLevelSize(texWidth, texHeight, 1, currentWriteLod);
        glDispatchCompute((mipLevelSize.x + 8 - 1) / 8, (mipLevelSize.y + 8 - 1) / 8, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    }
}

const char* Bloom::getName() const
{
    return "Bloom Pass";
}

void Bloom::renderUI()
{
#ifdef _MYDEBUG
    if(ImGui::SliderFloat("Threshold", &threshold, 0.0f, 10.0f))
    {
        AMC::bloom_threshold = threshold;
    }

    if(ImGui::SliderFloat("MaxColor", &maxColor, 0.0f, 3.0f))
    {
        AMC::bloom_maxcolor = maxColor;
    }

    int temp = minusLods;
    if (ImGui::SliderInt("MinusLods", &temp, 0, 10)) {
        minusLods = temp;
    }
    if (ImGui::CollapsingHeader("Bloom Texture")) {
        ImGui::Image((void*)(intptr_t)textureUpsample, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
    }
#endif
}
