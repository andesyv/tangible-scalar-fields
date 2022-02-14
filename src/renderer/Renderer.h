#pragma once

#include <unordered_set>
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
            struct SubShader {
                std::shared_ptr<globjects::File> m_file;
                std::shared_ptr<globjects::AbstractStringSource> m_source;
                std::unique_ptr<globjects::Shader> m_shader;
            };

            std::vector<SubShader> m_subshaders;
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
		/**
		 * Performs offscreen rendering on separate thread with separate context. All objects must be created in
		 * function scope, as it runs on the thread directly.
		 * @return Optional return true to indicate that offscreen work is done
		 */
        virtual bool offscreen_render() { return true; };
        /// Event run when data file is loaded
        virtual void fileLoaded(const std::string&);

        void addGlobalShaderInclude(const std::string& shaderIncludes);
		bool createShaderProgram(const std::string & name, std::initializer_list< std::pair<gl::GLenum, std::string> > shaders, std::initializer_list < std::string> shaderIncludes = {});
		static std::unique_ptr<globjects::Texture> create2DTexture(gl::GLenum tex, gl::GLenum minFilter, gl::GLenum magFilter, gl::GLenum wrapS, gl::GLenum wrapT, gl::GLint level, gl::GLenum internalFormat, const glm::ivec2 & size, gl::GLint border, gl::GLenum format, gl::GLenum type, const gl::GLvoid * data);
		globjects::Program* shaderProgram(const std::string & name);

    private:
		Viewer* m_viewer;
		bool m_enabled = true;
        std::map<std::string, std::pair<std::shared_ptr<globjects::AbstractStringSource>, std::shared_ptr<globjects::File>>> m_fileSources;
		std::unordered_map<std::string, ShaderProgram > m_shaderPrograms;
        std::map<std::string, std::pair<std::unique_ptr<globjects::File>, std::unique_ptr< globjects::NamedString>>> m_shaderIncludes;
	};
}