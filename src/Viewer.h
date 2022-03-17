#pragma once

#include <memory>
#include <vector>
#include <string>
#include <tuple>
#include <utility>
#include <map>
#include <tuple>
#include <functional>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace globjects{
    class File;
    class Shader;
    class Program;
    class Texture;
    class Buffer;
    class VertexArray;
    class Texture;
    class Renderbuffer;
    class Framebuffer;
}

/// Helper function that checks, in a tuple of weak_ptr's, if all weak_ptr's are valid
template <typename ... Ts>
bool all_t_weak_ptr(const std::tuple<Ts...>& tuple) {
    return std::apply([](Ts const&... tupleArgs){
        return (!tupleArgs.expired() && ...);
    }, tuple);
}

template <typename ... Ts>
auto initSubscribers(const std::tuple<Ts...>&) {
    return std::make_tuple(std::vector<std::function<void(Ts)>>{}...);
}

// https://stackoverflow.com/questions/18063451/get-index-of-a-tuple-elements-type
template <class T, class Tuple>
struct TupleIndex;

template <class T, class... Types>
struct TupleIndex<T, std::tuple<T, Types...>> {
    static const std::size_t value = 0;
};

template <class T, class U, class... Types>
struct TupleIndex<T, std::tuple<U, Types...>> {
    static const std::size_t value = 1 + TupleIndex<T, std::tuple<Types...>>::value;
};

namespace molumes
{
    class Tile;
    class Scene;
    class Renderer;
    class Interactor;

	class Viewer
	{
	public:
		Viewer(GLFWwindow* window, Scene* scene);
        ~Viewer();
		void display();
        bool offload_render();

		GLFWwindow * window();
		Scene* scene();

		glm::ivec2 viewportSize() const;

		glm::vec3 backgroundColor() const;
		glm::vec3 samplePointColor() const;
		glm::vec3 gridColor() const;
		glm::vec3 tileColor() const;
		glm::vec3 sobelEdgeColor() const;

		float scaleFactor() const;

		glm::mat4 modelTransform() const;
		glm::mat4 viewTransform() const;
		glm::mat4 lightTransform() const;
		glm::mat4 projectionTransform() const;

		glm::mat4 modelViewTransform() const;
		glm::mat4 modelViewProjectionTransform() const;
		glm::mat4 modelLightTransform() const;

		float m_scrollWheelSigma = 0.2f;
		float m_windowHeight = 720.0f;
		float m_windowWidth = 1280.0f;
		float m_scatterPlotDiameter = 2.0f*sqrt(3.0f);

		void setBackgroundColor(const glm::vec3& c);
		void setSamplePointColor(const glm::vec3& c);
		void setGridColor(const glm::vec3& c);
		void setTileColor(const glm::vec3& c);
		void setSobelEdgeColor(const glm::vec3& c);

		void setScaleFactor(const float s);

		void setViewTransform(const glm::mat4& m);
		void setModelTransform(const glm::mat4& m);
		void setLightTransform(const glm::mat4& m);
		void setProjectionTransform(const glm::mat4& m);

        std::vector<std::unique_ptr<Renderer>> &getRenderers();

        void enumerateFocusRenderer(bool inc = true);

		void saveImage(const std::string & filename) const;

        void openFile(const std::string& path = "");

        void setPerspective(bool bPerspective);

        /**
         * Forces the next frame to execute (at least) one offload render call, skipping waiting for an opening
         */
        void forceOffloadRender() { m_forceOffloadRender = true; }

		bool m_perspective = false;
        bool m_cameraRotateAllowed = false;
        bool m_forceOffloadRender = false;
        unsigned int m_focusRenderer = 0;

        /**
         * @brief Shared resources between renderers
         *
         * This is to make sure CrystalRenderer can read resources from the TileRenderer class without reading
         * invalid data or taking ownership of the resources while TileRenderer operates. Ownership still belongs
         * to TileRenderer. A quick and dirty "mediator" (mediator software design pattern).
         */
        struct SharedResources {
            std::weak_ptr<Tile> tile;
            std::weak_ptr<globjects::Texture> tileAccumulateTexture, smoothNormalsTexture;
            std::weak_ptr<globjects::Buffer> tileAccumulateMax, tileNormalsBuffer;

            [[nodiscard]] auto as_tuple() const {
                return std::make_tuple(tile, tileAccumulateTexture, smoothNormalsTexture, tileAccumulateMax, tileNormalsBuffer);
            }

            explicit operator bool() const { return all_t_weak_ptr(as_tuple()); }
        } m_sharedResources;

        using BroadcastHashType = std::size_t;
        using BroadcastTypes = std::tuple<glm::vec3, unsigned int, float, bool, int, std::vector<int>>;
        std::map<BroadcastHashType, decltype(initSubscribers(BroadcastTypes{}))> m_subscribers{};

        /**
         * @brief Broadcast value to subscriber functors.
         * Command pattern delegate implementation that uses the hash of a type as a identifier
         * @param hash_id The type hash identifier. Should be the type hash of the class member variable ptr.
         * (Use the molumes::getMemberVariableHash() helper function)
         * @param val The variable to pass down to subscribers
         */
        template <typename T>
        void broadcast(const BroadcastHashType& hash_id, T val) {
            static constexpr auto I = TupleIndex<T, BroadcastTypes>::value;
            auto& subs = std::get<I>(m_subscribers[hash_id]);
            for (auto& f : subs)
                f(val);
        }

    private:

		void beginFrame();
		void endFrame();
		void renderUi();
		void mainMenu();

		static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
		static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
		static void charCallback(GLFWwindow* window, unsigned int c);

		static const char* GetClipboardText(void* user_data);
		static void SetClipboardText(void* user_data, const char* text);

		GLFWwindow* m_window;
		Scene *m_scene;

		std::vector<std::unique_ptr<Interactor>> m_interactors;
		std::vector<std::unique_ptr<Renderer>> m_renderers;

        double m_time = 0.0;
		bool m_mousePressed[3] = { false, false, false };
		float m_mouseWheel = 0.0f;

        std::unique_ptr<globjects::Renderbuffer> m_offscreen_fb_color_rt, m_offscreen_fb_depth_rt;
        std::unique_ptr<globjects::Framebuffer> m_offscreen_fb;

		std::unique_ptr<globjects::File> m_vertexShaderSourceUi = nullptr;
		std::unique_ptr<globjects::File> m_fragmentShaderSourceUi = nullptr;

		std::unique_ptr<globjects::Shader> m_vertexShaderUi = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderUi = nullptr;
		std::unique_ptr<globjects::Program> m_programUi;

		std::unique_ptr<globjects::Texture> m_fontTexture = nullptr;
		std::unique_ptr<globjects::Buffer> m_verticesUi = nullptr;
		std::unique_ptr<globjects::Buffer> m_indicesUi = nullptr;
		std::unique_ptr<globjects::VertexArray> m_vaoUi = nullptr;

		// default colors for background and scatter plot sample points
		glm::vec3 m_backgroundColor = glm::vec3(0, 0, 0);				// black
		glm::vec3 m_samplePointColor = glm::vec3(0.5f, 0.5f, 0.5f);		// gray
		glm::vec3 m_gridColor = glm::vec3(0, 0, 0);						// black
		glm::vec3 m_tileColor = glm::vec3(1, 1, 1);						// white
		glm::vec3 m_sobelEdgeColor = glm::vec3(0.15f, 0.15f, 0.15f);	// gray

		float m_scaleFactor = 1.0f; //used to calculate correct size of elements whose size is set by the user

		glm::mat4 m_modelTransform = glm::mat4(1.0f);
		glm::mat4 m_viewTransform = glm::mat4(1.0f);
		glm::mat4 m_projectionTransform = glm::mat4(1.0f);
		glm::mat4 m_lightTransform = glm::mat4(1.0f);

		bool m_showUi = true;
		bool m_saveScreenshot = false;
	};
}