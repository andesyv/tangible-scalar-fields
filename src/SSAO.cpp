#include "SSAO.h"
#include <iostream>
#include "GFSDK_SSAO.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glbinding/gl/gl.h>

using namespace molumes;
using namespace glm;

template <class T>
void resolveFunction(T& function, const char* name)
{
	function = (T)glfwGetProcAddress(name);
}

SSAO::SSAO()
{
	GFSDK_SSAO_GLFunctions GL;
	resolveFunction(GL.glActiveTexture,"glActiveTexture"); 
	resolveFunction(GL.glAttachShader,"glAttachShader"); 
	resolveFunction(GL.glBindBuffer,"glBindBuffer"); 
	resolveFunction(GL.glBindBufferBase,"glBindBufferBase"); 
	resolveFunction(GL.glBindFramebuffer,"glBindFramebuffer"); 
	resolveFunction(GL.glBindFragDataLocation,"glBindFragDataLocation"); 
	resolveFunction(GL.glBindTexture,"glBindTexture"); 
	resolveFunction(GL.glBindVertexArray,"glBindVertexArray"); 
	resolveFunction(GL.glBlendColor,"glBlendColor"); 
	resolveFunction(GL.glBlendEquationSeparate,"glBlendEquationSeparate"); 
	resolveFunction(GL.glBlendFuncSeparate,"glBlendFuncSeparate"); 
	resolveFunction(GL.glBufferData,"glBufferData"); 
	resolveFunction(GL.glBufferSubData,"glBufferSubData"); 
	resolveFunction(GL.glColorMaski,"glColorMaski"); 
	resolveFunction(GL.glCompileShader,"glCompileShader"); 
	resolveFunction(GL.glCreateShader,"glCreateShader"); 
	resolveFunction(GL.glCreateProgram,"glCreateProgram"); 
	resolveFunction(GL.glDeleteBuffers,"glDeleteBuffers"); 
	resolveFunction(GL.glDeleteFramebuffers,"glDeleteFramebuffers"); 
	resolveFunction(GL.glDeleteProgram,"glDeleteProgram"); 
	resolveFunction(GL.glDeleteShader,"glDeleteShader"); 
	resolveFunction(GL.glDeleteTextures,"glDeleteTextures"); 
	resolveFunction(GL.glDeleteVertexArrays,"glDeleteVertexArrays"); 
	resolveFunction(GL.glDisable,"glDisable"); 
	resolveFunction(GL.glDrawBuffers,"glDrawBuffers"); 
	resolveFunction(GL.glEnable,"glEnable"); 
	resolveFunction(GL.glDrawArrays,"glDrawArrays"); 
	resolveFunction(GL.glFramebufferTexture,"glFramebufferTexture"); 
	resolveFunction(GL.glFramebufferTexture2D,"glFramebufferTexture2D"); 
	resolveFunction(GL.glFramebufferTextureLayer,"glFramebufferTextureLayer"); 
	resolveFunction(GL.glGenBuffers,"glGenBuffers"); 
	resolveFunction(GL.glGenFramebuffers,"glGenFramebuffers"); 
	resolveFunction(GL.glGenTextures,"glGenTextures"); 
	resolveFunction(GL.glGenVertexArrays,"glGenVertexArrays"); 
	resolveFunction(GL.glGetError,"glGetError"); 
	resolveFunction(GL.glGetBooleani_v,"glGetBooleani_v"); 
	resolveFunction(GL.glGetFloatv,"glGetFloatv"); 
	resolveFunction(GL.glGetIntegerv,"glGetIntegerv"); 
	resolveFunction(GL.glGetIntegeri_v,"glGetIntegeri_v"); 
	resolveFunction(GL.glGetProgramiv,"glGetProgramiv"); 
	resolveFunction(GL.glGetProgramInfoLog,"glGetProgramInfoLog"); 
	resolveFunction(GL.glGetShaderiv,"glGetShaderiv"); 
	resolveFunction(GL.glGetShaderInfoLog,"glGetShaderInfoLog"); 
	resolveFunction(GL.glGetString,"glGetString"); 
	resolveFunction(GL.glGetUniformBlockIndex,"glGetUniformBlockIndex"); 
	resolveFunction(GL.glGetUniformLocation,"glGetUniformLocation"); 
	resolveFunction(GL.glGetTexLevelParameteriv,"glGetTexLevelParameteriv"); 
	resolveFunction(GL.glIsEnabled,"glIsEnabled"); 
	resolveFunction(GL.glIsEnabledi,"glIsEnabledi"); 
	resolveFunction(GL.glLinkProgram,"glLinkProgram"); 
	resolveFunction(GL.glPolygonOffset,"glPolygonOffset"); 
	resolveFunction(GL.glShaderSource,"glShaderSource"); 
	resolveFunction(GL.glTexImage2D,"glTexImage2D"); 
	resolveFunction(GL.glTexImage3D,"glTexImage3D"); 
	resolveFunction(GL.glTexParameteri,"glTexParameteri"); 
	resolveFunction(GL.glTexParameterfv,"glTexParameterfv"); 
	resolveFunction(GL.glUniform1i,"glUniform1i"); 
	resolveFunction(GL.glUniformBlockBinding,"glUniformBlockBinding"); 
	resolveFunction(GL.glUseProgram,"glUseProgram"); 
	resolveFunction(GL.glViewport,"glViewport"); 

	GFSDK_SSAO_CustomHeap customHeap;
	customHeap.new_ = ::operator new;
	customHeap.delete_ = ::operator delete;
	
	GFSDK_SSAO_Status status = GFSDK_SSAO_CreateContext_GL(&m_aoContext, &GL, &customHeap);

	if (status == GFSDK_SSAO_OK)
	{
		std::cerr << "SSAO initialized successfully!" << std::endl;
	}
	else
	{
		std::cerr << "SSAO could not be initialized!" << std::endl;
	}
}

void SSAO::display(const glm::mat4 & viewTransform, const glm::mat4 & projectionTransform, glm::uint frameBuffer, glm::uint depthTexture, glm::uint normalTexture)
{
	GFSDK_SSAO_Parameters params;
	params.SmallScaleAO = 1.0f;
	params.LargeScaleAO = 1.0f;
	params.Radius = 1.0;
	params.PowerExponent = 2.0;
	params.Bias = 0.0f;
	params.Blur.Enable = true;
	params.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_4;
	params.Blur.Sharpness = 8.0f;
//	params.ForegroundAO.Enable = true;
//	params.BackgroundAO.Enable = true;

	GFSDK_SSAO_InputData_GL input;
	input.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
	input.DepthData.FullResDepthTexture = GFSDK_SSAO_Texture_GL((GLenum)gl::GL_TEXTURE_2D, depthTexture);
	input.DepthData.ProjectionMatrix.Data = (const GFSDK_SSAO_FLOAT*)&projectionTransform;
	input.DepthData.MetersToViewSpaceUnits = 1.0f;
	input.DepthData.ProjectionMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;

	
	input.NormalData.Enable = true;
	input.NormalData.FullResNormalTexture = GFSDK_SSAO_Texture_GL((GLenum)gl::GL_TEXTURE_2D, normalTexture);
	input.NormalData.WorldToViewMatrix.Data = (const GFSDK_SSAO_FLOAT*)&viewTransform;
	//input.NormalData.WorldToViewMatrix.Layout = GFSDK_SSAO_COLUMN_MAJOR_ORDER;
	
	GFSDK_SSAO_Output_GL output;
	output.Blend.Mode = GFSDK_SSAO_MULTIPLY_RGB;
	output.OutputFBO = frameBuffer;
/*
	if (normalBuffer)
	{
		const float4x4& view = viewCamera.GetViewMatrix();
		GLuint normalOGL = (static_cast_checked<OpenGLTexture*>(normalBuffer.get()))->GetTextureOGL();

		Input.NormalData.Enable = true;
		Input.NormalData.FullResNormalTexture = GFSDK_SSAO_Texture_GL(GL_TEXTURE_2D, normalOGL);
		Input.NormalData.pWorldToViewMatrix = (const GFSDK_SSAO_FLOAT*)&view;
		Input.NormalData.WorldToViewMatrixLayout = GFSDK_SSAO_ROW_MAJOR_ORDER;
		Input.NormalData.DecodeScale = 2.f;
		Input.NormalData.DecodeBias = -1.f;
	}
*/
	
	GFSDK_SSAO_Status status;
	status = m_aoContext->RenderAO(input, params, output, GFSDK_SSAO_RENDER_AO);
	
	if (status != GFSDK_SSAO_OK)
		std::cerr << "AO rendering failed!" << std::endl;
}
