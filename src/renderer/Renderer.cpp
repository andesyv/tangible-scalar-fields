#include "Renderer.h"
#include <globjects/base/File.h>
#include <globjects/State.h>
#include <iostream>
#include <filesystem>

#include <list>
#include <utility>
#include <initializer_list>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glbinding/gl/gl.h>

#include <globjects/VertexArray.h>
#include <globjects/Buffer.h>
#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <globjects/Texture.h>
#include <globjects/NamedString.h>
#include <globjects/base/StaticStringSource.h>

using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;

Renderer::Renderer(Viewer *viewer) : m_viewer(viewer) {

}

Viewer *Renderer::viewer() {
    return m_viewer;
}

void Renderer::setEnabled(bool enabled) {
    m_enabled = enabled;
}

bool Renderer::isEnabled() const {
    return m_enabled;
}

void Renderer::reloadShaders() {
    for (auto&[key, val]: m_fileSources) {
        globjects::debug() << "Reloading shader file " << val.second->filePath() << " ...";
        val.second->reload();
    }

    for (auto &p: m_shaderPrograms) {
        globjects::debug() << "Reloading shader program " << p.first << " ...";

        for (auto &s: p.second.m_subshaders) {
            if (!s.m_shader->compile())
                globjects::critical() << "Shader '" << s.m_file->filePath() << "' failed to compile!";
        }
        p.second.m_program->link();
    }
}

bool
Renderer::createShaderProgram(const std::string &name, std::initializer_list<std::pair<GLenum, std::string> > shaders,
                              std::initializer_list<std::string> shaderIncludes) {
    globjects::debug() << "Creating shader program " << name << " ...";

    /// Aggregate initialization is pretty neat:
    ShaderProgram program{.m_program = std::move(Program::create())};
    bool success = true;

    for (const auto &i: shaderIncludes)
        addGlobalShaderInclude(i);

    for (const auto&[type, path]: shaders) {
        globjects::debug() << "Loading shader file " << path << " ...";

        std::shared_ptr file{Shader::sourceFromFile(path)};
        if (file->string().empty())
            continue;
        std::shared_ptr source{Shader::applyGlobalReplacements(file.get())};

        auto[actual_source, actual_file] = m_fileSources.emplace(file->filePath(),
                                                                 std::make_pair(source, file)).first->second;

        auto shader = Shader::create(type, actual_source.get());

        if (!shader->compile()) {
            globjects::critical() << "Shader '" << path << "' failed to compile!";
            success = false;
        }
        program.m_program->attach(shader.get());
        program.m_subshaders.push_back({
                                               .m_file = std::move(actual_file),
                                               .m_source = std::move(actual_source),
                                               .m_shader = std::move(shader)
                                       });
    }

    program.m_program->link();

    m_shaderPrograms[name] = std::move(program);

    return success;
}

std::unique_ptr<globjects::Texture>
Renderer::create2DTexture(GLenum tex, GLenum minFilter, GLenum magFilter, GLenum wrapS, GLenum wrapT,
                          gl::GLint level, gl::GLenum internalFormat, const glm::ivec2 &size, gl::GLint border,
                          gl::GLenum format, gl::GLenum type, const gl::GLvoid *data) {

    std::unique_ptr<globjects::Texture> texture = Texture::create(tex);
    texture->setParameter(GL_TEXTURE_MIN_FILTER, minFilter);
    texture->setParameter(GL_TEXTURE_MAG_FILTER, magFilter);
    texture->setParameter(GL_TEXTURE_WRAP_S, wrapS);
    texture->setParameter(GL_TEXTURE_WRAP_T, wrapT);
    texture->image2D(level, internalFormat, size, border, format, type, data);

    return texture;
}

globjects::Program *Renderer::shaderProgram(const std::string &name) {
    auto &p = m_shaderPrograms.at(name).m_program;
    return p->isValid() ? p.get() : nullptr;
}

void Renderer::fileLoaded(const std::string &) {}

void Renderer::addGlobalShaderInclude(const std::string &str) {
    globjects::debug() << "Loading include file " << str << " ...";

    std::filesystem::path path{str};
    std::string shaderIncludeName = "/" + path.filename().string();
    if (m_shaderIncludes.contains(shaderIncludeName))
        return;

    auto file = File::create(str);
    auto string = NamedString::create(shaderIncludeName, file.get());

    m_shaderIncludes.insert(std::make_pair(shaderIncludeName, std::make_pair(std::move(file), std::move(string))));
}

// Trivial destructor explicitly defined in source file purely to skip including definitions in header
Renderer::~Renderer() = default;

