#pragma once

#include<common.h>
#include<ShaderProgram.h>
#include<Model.h>
#include<ModelPlacer.h>
#include<Scene.h>
#include<SplineCameraAdjuster.h>
#include<EventManager.h>

class AMCBannerScene : public AMC::Scene {

private:
	AMC::ModelPlacer* mp;
	AMC::EventManager* events;

	// Defined Callbacks for events
	void sceneEnd(float);
	void RenderBanner1(float);
	void RenderBanner2(float);

public:
	GLuint textureBanner1;
	GLuint textureBanner2;
	GLuint outputTex;
	void init() override;
	//void render() override;
	void renderDebug() override;
	void renderUI() override;
	void update() override;
	void keyboardfunc(char key, UINT keycode) override;
	void updateRenderContext(AMC::RenderContext& context) override;
	AMC::Camera* getCamera() override;
};
