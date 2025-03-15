#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <chrono>
#include "Math/Vector.h"
#include "Math/Quat.h"
#include "Shape.h"
#include "Body.h"
#include "Renderer/DeviceContext.h"
#include "Renderer/model.h"
#include "Renderer/shader.h"
#include "Renderer/FrameBuffer.h"

class Application
{
public:
	Application() : m_isPaused( false ), m_frameStep( false ) {}
	~Application();
	void Initialize();
	void MainLoop();

private:
	std::vector< const char * > GetGLFWRequiredExtensions() const;
	void InitializeGLFW();
	bool InitializeVulkan();
	void Cleanup();
	void UpdateUniforms();
	void DrawFrame();
	void ResizeWindow( int windowWidth, int windowHeight );
	void MouseMoved( float x, float y );
	void MouseScrolled( float z );
	void Keyboard( int key, int scancode, int action, int modifiers );
	void MouseButton(int button, int action, int mods);
	static void OnWindowResized( GLFWwindow * window, int width, int height );
	static void OnMouseMoved( GLFWwindow * window, double x, double y );
	static void OnMouseWheelScrolled( GLFWwindow * window, double x, double y );
	static void OnKeyboard( GLFWwindow * window, int key, int scancode, int action, int modifiers );
	static void OnMouseButton(GLFWwindow* window, int button, int action, int mods);
	class Scene * scene;
	GLFWwindow * glfwWindow;
	DeviceContext deviceContext;
	Buffer m_uniformBuffer;
	Model m_modelFullScreen;
	std::vector< Model * > m_models;	
	Shader		m_copyShader;
	Descriptors	m_copyDescriptors;
	Pipeline	m_copyPipeline;
	Vec2 m_mousePosition;
	Vec3 m_cameraFocusPoint;
	Vec3 m_camPos;
	float m_cameraPositionT;
	float m_cameraPositionP;
	float m_cameraRadius;
	bool m_isPaused;
	bool m_frameStep;
	std::vector< RenderModel > m_renderModels;
	static const int WINDOW_WIDTH = 1200;
	static const int WINDOW_HEIGHT = 720;
	static const bool m_enableLayers = true;
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point end;
};

extern Application * application;