#pragma once

#include<GL/glew.h>
#include<vector>
#include<Log.h>
#include<Light.h>
#include<ShadowManager.h>
#include<ShaderProgram.h>
#include<Model.h>
#include<glm/glm.hpp>

namespace AMC {

    struct alignas(16) GpuLight {
        glm::vec3 position; float intensity;
        glm::vec3 direction; float range;
        glm::vec3 color; float spotAngle;
        float spotExponent;
        int type = 2;
        int shadows = 0;
        int shadowMapIndex = -1;
        int active;
        float pad;
    };

    struct alignas(16) LightBlock {
        GpuLight u_Lights[MAX_LIGHTS];
        int u_LightCount; int pad0; int pad1; int pad2;
    };

    class LightManager {
        public:
            LightManager(int maxShadowMaps, int maxPointShadowCubemaps);

            void addLight(const Light& l);
            void removeLight(int index);
            Light* getLight(int index);
            std::vector<Light*> getShadowCastingLights();

            ShadowManager* getShadowMapManager();
            void toggleLightShadow(Light& light, bool enable);
            void updateUBO();
            void bindUBO();

            // For UI and debugging
            void renderUI();
            void drawLights();

            std::vector<Light> lights;
        private:
            ShadowManager *shadowManager = nullptr;
            GLuint uboLights = 0;
            static ShaderProgram* m_program;
            static Model *directional, *spot, *point;
    };


};
