#include <thread>
#include "Renderer/DeviceContext.h"
#include "Renderer/model.h"
#include "Renderer/shader.h"
#include "Renderer/Samplers.h"
#include "application.h"
#include "Fileio.h"
#include <assert.h>
#include "Renderer/OffscreenRenderer.h"
#include "Scene.h"

Application * application = NULL;

#include <time.h>
#include <windows.h>

static bool gIsInitialized( false );
static unsigned __int64 gTicksPerSecond;
static unsigned __int64 gStartTicks;

int GetTimeMicroseconds()
{
	if ( false == gIsInitialized )
	{
		gIsInitialized = true;
		QueryPerformanceFrequency( (LARGE_INTEGER *)&gTicksPerSecond );
		QueryPerformanceCounter( (LARGE_INTEGER *)&gStartTicks );
		return 0;
	}

	unsigned __int64 tick;
	QueryPerformanceCounter( (LARGE_INTEGER *)&tick );
	const double ticks_per_micro = (double)( gTicksPerSecond / 1000000 );
	const unsigned __int64 timeMicro = (unsigned __int64)( (double)( tick - gStartTicks ) / ticks_per_micro );
	return (int)timeMicro;
}

void Application::Initialize()
{
	InitializeGLFW();
	InitializeVulkan();
	scene = new Scene;
	scene->Initialize();
	scene->Reset();
	models_.reserve( scene->bodies.size() );
	
	for ( int i = 0; i < scene->bodies.size(); i++ )
	{
		Model * model = new Model();
		model->BuildFromShape( scene->bodies[ i ]->shape );
		model->MakeVBO( &deviceContext );
		models_.push_back( model );
	}

	mouse_position_ = Vec2( 0, 0 );
	camera_position_t_ = acosf( -1.0f ) / 2.0f;
	camera_position_p_ = 0;
	camera_radius_ = 20.0f;
	camera_focus_point_ = Vec3( 0, 0, 3 );
	is_paused_ = false;
	frame_step_ = false;
}

Application::~Application()
{
	Cleanup();
}

void Application::InitializeGLFW()
{
	glfwInit();
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindow = glfwCreateWindow( WINDOW_WIDTH, WINDOW_HEIGHT, "Petanque Physics", nullptr, nullptr );
	glfwSetWindowUserPointer( glfwWindow, this );
	glfwSetWindowSizeCallback( glfwWindow, Application::OnWindowResized );
	glfwSetInputMode( glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
	glfwSetInputMode( glfwWindow, GLFW_STICKY_KEYS, GLFW_TRUE );
	glfwSetCursorPosCallback( glfwWindow, Application::OnMouseMoved );
	glfwSetScrollCallback( glfwWindow, Application::OnMouseWheelScrolled );
	glfwSetKeyCallback( glfwWindow, Application::OnKeyboard );
	glfwSetMouseButtonCallback(glfwWindow, Application::OnMouseButton);
}

std::vector< const char * > Application::GetGLFWRequiredExtensions() const
{
	std::vector< const char * > extensions;
	const char ** glfwExtensions;
	uint32_t glfwExtensionCount = 0;
	glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
	for ( uint32_t i = 0; i < glfwExtensionCount; i++ )
	{
		extensions.push_back( glfwExtensions[ i ] );
	}
	if ( m_enableLayers )
	{
		extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
	}
	return extensions;
}

bool Application::InitializeVulkan()
{
	{
		std::vector< const char * > extensions = GetGLFWRequiredExtensions();
		if ( !deviceContext.CreateInstance( m_enableLayers, extensions ) )
		{
			printf( "Failed to create vulkan instance\n" );
			assert( 0 );
			return false;
		}
	}

	if ( VK_SUCCESS != glfwCreateWindowSurface( deviceContext.m_vkInstance, glfwWindow, nullptr, &deviceContext.m_vkSurface ) )
	{
		printf( "Failed to create window surface\n" );
		assert( 0 );
		return false;
	}

	int windowWidth;
	int windowHeight;
	glfwGetWindowSize( glfwWindow, &windowWidth, &windowHeight );
	
	if ( !deviceContext.CreateDevice() )
	{
		printf( "Failed to create device\n" );
		assert( 0 );
		return false;
	}
	if ( !deviceContext.CreateSwapChain( windowWidth, windowHeight ) )
	{
		printf( "Failed to create swapchain\n" );
		assert( 0 );
		return false;
	}

	Samplers::InitializeSamplers( &deviceContext );

	if ( !deviceContext.CreateCommandBuffers() )
	{
		printf( "Failed to create command buffers\n" );
		assert( 0 );
		return false;
	}

	uniform_buffer_.Allocate( &deviceContext, NULL, sizeof( float ) * 16 * 4 * 128, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT );
	InitOffscreen( &deviceContext, deviceContext.swap_chain.window_width, deviceContext.swap_chain.window_height );

	{
		bool result;
		FillFullScreenQuad( model_full_screen_ );
		for ( int i = 0; i < model_full_screen_.vertices.size(); i++ )
		{
			model_full_screen_.vertices[ i ].xyz[ 1 ] *= -1.0f;
		}
		model_full_screen_.MakeVBO( &deviceContext );
		result = copy_shader_.Load( &deviceContext, "DebugImage2D" );
		if ( !result )
		{
			printf( "Failed to load copy shader\n" );
			assert( 0 );
			return false;
		}

		Descriptors::CreateParms_t descriptorParms;
		memset( &descriptorParms, 0, sizeof( descriptorParms ) );
		descriptorParms.numUniformsFragment = 1;
		descriptorParms.numImageSamplers = 1;
		result = copy_descriptors_.Create( &deviceContext, descriptorParms );
		if ( !result )
		{
			printf( "Failed to create copy descriptors\n" );
			assert( 0 );
			return false;
		}

		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.renderPass = deviceContext.swap_chain.m_vkRenderPass;
		pipelineParms.descriptors = &copy_descriptors_;
		pipelineParms.shader = &copy_shader_;
		pipelineParms.width = deviceContext.swap_chain.window_width;
		pipelineParms.height = deviceContext.swap_chain.window_height;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		result = copy_pipeline_.Create( &deviceContext, pipelineParms );
		if ( !result )
		{
			printf( "Failed to create copy pipeline\n" );
			assert( 0 );
			return false;
		}
	}

	return true;
}

void Application::Cleanup()
{
	CleanupOffscreen( &deviceContext );

	copy_shader_.Cleanup( &deviceContext );
	copy_descriptors_.Cleanup( &deviceContext );
	copy_pipeline_.Cleanup( &deviceContext );
	model_full_screen_.Cleanup( deviceContext );
	
	delete scene;
	scene = NULL;
	
	for ( int i = 0; i < models_.size(); i++ )
	{
		models_[ i ]->Cleanup( deviceContext );
		delete models_[ i ];
	}
	models_.clear();
	uniform_buffer_.Cleanup( &deviceContext );
	Samplers::Cleanup( &deviceContext );
	deviceContext.Cleanup();
	glfwDestroyWindow( glfwWindow );
	glfwTerminate();
}

void Application::OnWindowResized( GLFWwindow * window, int windowWidth, int windowHeight )
{
	if ( 0 == windowWidth || 0 == windowHeight )
	{
		return;
	}

	Application * application = reinterpret_cast< Application * >( glfwGetWindowUserPointer( window ) );
	application->ResizeWindow( windowWidth, windowHeight );
}

void Application::ResizeWindow( int windowWidth, int windowHeight )
{
	deviceContext.ResizeWindow( windowWidth, windowHeight );
	{
		bool result;
		copy_pipeline_.Cleanup( &deviceContext );
		Pipeline::CreateParms_t pipelineParms;
		memset( &pipelineParms, 0, sizeof( pipelineParms ) );
		pipelineParms.renderPass = deviceContext.swap_chain.m_vkRenderPass;
		pipelineParms.descriptors = &copy_descriptors_;
		pipelineParms.shader = &copy_shader_;
		pipelineParms.width = windowWidth;
		pipelineParms.height = windowHeight;
		pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
		pipelineParms.depthTest = false;
		pipelineParms.depthWrite = false;
		result = copy_pipeline_.Create( &deviceContext, pipelineParms );
		if ( !result )
		{
			printf( "Unable to build pipeline!\n" );
			assert( 0 );
			return;
		}
	}
}

void Application::OnMouseMoved( GLFWwindow * window, double x, double y )
{
	Application * application = reinterpret_cast< Application * >( glfwGetWindowUserPointer( window ) );
	application->MouseMoved( (float)x, (float)y );
}

void Application::MouseMoved( float x, float y )
{
	Vec2 newPosition = Vec2( x, y );
	Vec2 ds = newPosition - mouse_position_;
	mouse_position_ = newPosition;
	float sensitivity = 0.01f;
	camera_position_t_ += ds.y * -sensitivity;
	camera_position_p_ += ds.x * -sensitivity;
	
	if ( camera_position_t_ < 0.14f )
	{
		camera_position_t_ = 0.14f;
	}
	if ( camera_position_t_ > 1.7f )
	{
		camera_position_t_ = 1.7f;
	}

	if ( camera_position_p_ < -1.57f )
	{
		camera_position_p_ = -1.57f;
	}
	if ( camera_position_p_ > 1.57f )
	{
		camera_position_p_ = 1.57f;
	}
}

void Application::OnMouseWheelScrolled( GLFWwindow * window, double x, double y )
{
	Application * application = reinterpret_cast< Application *
		>( glfwGetWindowUserPointer( window ) );
	application->MouseScrolled( (float)y );
}

void Application::MouseScrolled( float z )
{

}

void Application::OnKeyboard( GLFWwindow * window, int key, int scancode, int action, int modifiers )
{
	Application * application = reinterpret_cast< Application *
		>( glfwGetWindowUserPointer( window ) );
	application->Keyboard( key, scancode, action, modifiers );
}

void Application::OnMouseButton(GLFWwindow* window, int button, int action, int mods)
{
	Application * application = reinterpret_cast< Application *
		>( glfwGetWindowUserPointer( window ) );
	application->MouseButton( button, action, mods );
}

void Application::MouseButton(int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		start = std::chrono::high_resolution_clock::now();
	}
	
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> duration = end - start;
		scene->BallSpawn(cam_pos_, camera_focus_point_, duration.count());
	}
}

void Application::Keyboard( int key, int scancode, int action, int modifiers )
{

}

void Application::MainLoop()
{
	static int timeLastFrame = 0;
	static int numSamples = 0;
	static float avgTime = 0.0f;
	static float maxTime = 0.0f;

	while ( !glfwWindowShouldClose( glfwWindow ) )
	{
		int time = GetTimeMicroseconds();
		float dt_us	= (float)time - (float)timeLastFrame;
		if ( dt_us < 16000.0f )
		{
			int x = 16000 - (int)dt_us;
			std::this_thread::sleep_for( std::chrono::microseconds( x ) );
			dt_us = 16000;
			time = GetTimeMicroseconds();
		}
		timeLastFrame = time;
		glfwPollEvents();
		
		if ( dt_us > 33000.0f )
		{
			dt_us = 33000.0f;
		}

		bool runPhysics = true;
		if ( is_paused_ )
		{
			dt_us = 0.0f;
			runPhysics = false;
			if ( frame_step_ )
			{
				dt_us = 16667.0f;
				frame_step_ = false;
				runPhysics = true;
			}
			numSamples = 0;
			maxTime = 0.0f;
		}
		float dt_sec = dt_us * 0.001f * 0.001f;
		
		if ( runPhysics )
		{
			int startTime = GetTimeMicroseconds();
			for ( int i = 0; i < 2; i++ )
			{
				scene->Update( dt_sec * 0.5f );
			}
			int endTime = GetTimeMicroseconds();
			dt_us = (float)endTime - (float)startTime;
			if ( dt_us > maxTime )
			{
				maxTime = dt_us;
			}

			avgTime = ( avgTime * float( numSamples ) + dt_us ) / float( numSamples + 1 );
			numSamples++;

		}
		DrawFrame();
		if(scene->EndUpdate())
		{
			models_.clear();
			models_.reserve( scene->bodies.size() );
			for ( int i = 0; i < scene->bodies.size(); i++ )
			{
				Model * model = new Model();
				model->BuildFromShape( scene->bodies[ i ]->shape );
				model->MakeVBO( &deviceContext );
				models_.push_back( model );
			}
		}
	}
}

void Application::UpdateUniforms()
{
	m_renderModels.clear();
	uint32_t uboByteOffset = 0;
	uint32_t cameraByteOFfset = 0;
	uint32_t shadowByteOffset = 0;
	struct camera_t
	{
		Mat4 matView;
		Mat4 matProj;
		Mat4 pad0;
		Mat4 pad1;
	};
	camera_t camera;

	{
		unsigned char * mappedData = (unsigned char *)uniform_buffer_.MapBuffer( &deviceContext );
		{
			cam_pos_ = Vec3( 10, 0, 5 ) * 1.25f;
			Vec3 camLookAt = Vec3( 0, 0, 1 );
			Vec3 camUp = Vec3( 0, 0, 1 );
			cam_pos_.x = cosf( camera_position_p_ ) * sinf( camera_position_t_ );
			cam_pos_.y = sinf( camera_position_p_ ) * sinf( camera_position_t_ );
			cam_pos_.z = cosf( camera_position_t_ );
			cam_pos_ *= camera_radius_;
			cam_pos_ += camera_focus_point_;
			camLookAt = camera_focus_point_;

			int windowWidth;
			int windowHeight;
			glfwGetWindowSize( glfwWindow, &windowWidth, &windowHeight );
			const float zNear   = 0.1f;
			const float zFar    = 1000.0f;
			const float fovy	= 45.0f;
			const float aspect	= (float)windowHeight / (float)windowWidth;
			camera.matProj.PerspectiveVulkan( fovy, aspect, zNear, zFar );
			camera.matProj = camera.matProj.Transpose();
			camera.matView.LookAt( cam_pos_, camLookAt, camUp );
			camera.matView = camera.matView.Transpose();
			memcpy( mappedData + uboByteOffset, &camera, sizeof( camera ) );
			cameraByteOFfset = uboByteOffset;
			uboByteOffset += deviceContext.GetAligendUniformByteOffset( sizeof( camera ) );
		}

		{
			Vec3 camPos = Vec3( 1, 1, 1 ) * 75.0f;
			Vec3 camLookAt = Vec3( 0, 0, 0 );
			Vec3 camUp = Vec3( 0, 0, 1 );
			Vec3 tmp = camPos.Cross( camUp );
			camUp = tmp.Cross( camPos );
			camUp.Normalize();
			extern FrameBuffer g_shadowFrameBuffer;
			const int windowWidth = g_shadowFrameBuffer.m_parms.width;
			const int windowHeight = g_shadowFrameBuffer.m_parms.height;
			const float halfWidth = 60.0f;
			const float xmin = -halfWidth;
			const float xmax = halfWidth;
			const float ymin = -halfWidth;
			const float ymax = halfWidth;
			const float zNear = 25.0f;
			const float zFar = 175.0f;
			camera.matProj.OrthoVulkan( xmin, xmax, ymin, ymax, zNear, zFar );
			camera.matProj = camera.matProj.Transpose();
			camera.matView.LookAt( camPos, camLookAt, camUp );
			camera.matView = camera.matView.Transpose();
			memcpy( mappedData + uboByteOffset, &camera, sizeof( camera ) );
			shadowByteOffset = uboByteOffset;
			uboByteOffset += deviceContext.GetAligendUniformByteOffset( sizeof( camera ) );
		}

		for ( int i = 0; i < scene->bodies.size(); i++ )
		{
			Body & body = *scene->bodies[ i ];
			Vec3 fwd = body.orientation.RotatePoint( Vec3( 1, 0, 0 ) );
			Vec3 up = body.orientation.RotatePoint( Vec3( 0, 0, 1 ) );
			Mat4 matOrient;
			matOrient.Orient( body.position, fwd, up );
			matOrient = matOrient.Transpose();
			memcpy( mappedData + uboByteOffset, matOrient.ToPtr(), sizeof( matOrient ) );
			RenderModel renderModel;
			renderModel.model = models_[ i ];
			renderModel.uboByteOffset = uboByteOffset;
			renderModel.uboByteSize = sizeof( matOrient );
			renderModel.pos = body.position;
			renderModel.orient = body.orientation;
			m_renderModels.push_back( renderModel );
			uboByteOffset += deviceContext.GetAligendUniformByteOffset( sizeof( matOrient ) );
		}
		uniform_buffer_.UnmapBuffer( &deviceContext );
	}
}

void Application::DrawFrame()
{
	UpdateUniforms();
	const uint32_t imageIndex = deviceContext.BeginFrame();
	DrawOffscreen( &deviceContext, imageIndex, &uniform_buffer_, m_renderModels.data(), (int)m_renderModels.size() );
 	deviceContext.BeginRenderPass();
	{
		extern FrameBuffer g_offscreenFrameBuffer;
		VkCommandBuffer cmdBuffer = deviceContext.m_vkCommandBuffers[ imageIndex ];
		copy_pipeline_.BindPipeline( cmdBuffer );
		Descriptor descriptor = copy_pipeline_.GetFreeDescriptor();
		descriptor.BindImage( VK_IMAGE_LAYOUT_GENERAL, g_offscreenFrameBuffer.m_imageColor.m_vkImageView, Samplers::m_samplerStandard, 0 );
		descriptor.BindDescriptor( &deviceContext, cmdBuffer, &copy_pipeline_ );
		model_full_screen_.DrawIndexed( cmdBuffer );
	}
 	deviceContext.EndRenderPass();
	deviceContext.EndFrame();
}