#pragma once

#include <set>
#include <memory>
#include <string>
#include <unordered_map>
#include <map>

#include <glbinding/gl/types.h>
#include <glm/fwd.hpp>

namespace globjects{
    class File;
    class Shader;
    class Program;
    class Texture;
    class AbstractStringSource;
    class NamedString;
}

namespace molumes
{
	class Viewer;

	class Renderer
	{

		struct ShaderProgram
		{
			std::set< std::unique_ptr< globjects::File> > m_files;
			std::set< std::unique_ptr< globjects::AbstractStringSource> > m_sources;
			std::set< std::unique_ptr< globjects::Shader > > m_shaders;
			std::unique_ptr< globjects::Program > m_program;
 		};


	public:
		Renderer(Viewer* viewer);
        virtual ~Renderer();
		Viewer * viewer();
		virtual void setEnabled(bool enabled);
		bool isEnabled() const;
		virtual void reloadShaders();
		virtual void display() = 0;
        /// Event run when data file is loaded
        virtual void fileLoaded(const std::string&);

        void addGlobalShaderInclude(const std::string& shaderIncludes);
		bool createShaderProgram(const std::string & name, std::initializer_list< std::pair<gl::GLenum, std::string> > shaders, std::initializer_list < std::string> shaderIncludes = {});
		static std::unique_ptr<globjects::Texture> create2DTexture(gl::GLenum tex, gl::GLenum minFilter, gl::GLenum magFilter, gl::GLenum wrapS, gl::GLenum wrapT, gl::GLint level, gl::GLenum internalFormat, const glm::ivec2 & size, gl::GLint border, gl::GLenum format, gl::GLenum type, const gl::GLvoid * data);
		globjects::Program* shaderProgram(const std::string & name);

    private:
		Viewer* m_viewer;
		bool m_enabled = true;
		std::unordered_map<std::string, ShaderProgram > m_shaderPrograms;
        std::map<std::string, std::pair<std::unique_ptr< globjects::NamedString>, std::unique_ptr<globjects::File>>> m_shaderIncludes;
	};

}