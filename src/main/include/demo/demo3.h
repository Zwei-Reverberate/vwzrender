#pragma once
#include <demo/demobase.h>
#include <string>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace demo
{
	constexpr float kEps = 1e-5f;

	std::string formatVec3(const glm::vec3& v);
	std::string formatVec4(const glm::vec4& v);
	glm::vec3 transformPoint(const glm::mat4& m, const glm::vec3& p);

	struct MathCheck 
	{
		std::uint32_t total{ 0 };
		std::uint32_t passed{ 0 };

		void record(std::string_view name, bool ok, const std::string& detail);
		void number(std::string_view name, float actual, float expected);
		void vec3(std::string_view name, const glm::vec3& actual, const glm::vec3& expected);
		void vec4(std::string_view name, const glm::vec4& actual, const glm::vec4& expected);
	};

	struct CubeEdge
	{
		std::uint8_t a;
		std::uint8_t b;
	};

	constexpr std::array<CubeEdge, 12> kCubeEdges = 
	{
		CubeEdge{ 0, 1 }, CubeEdge{ 1, 2 }, CubeEdge{ 2, 3 }, CubeEdge{ 3, 0 },
		CubeEdge{ 4, 5 }, CubeEdge{ 5, 6 }, CubeEdge{ 6, 7 }, CubeEdge{ 7, 4 },
		CubeEdge{ 0, 4 }, CubeEdge{ 1, 5 }, CubeEdge{ 2, 6 }, CubeEdge{ 3, 7 },
	};

	constexpr std::array<glm::vec3, 8> kCubeVertices = 
	{
	    glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.5f, -0.5f, -0.5f),
	    glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(-0.5f, 0.5f, -0.5f),
	    glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0.5f, -0.5f, 0.5f),
	    glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, 0.5f),
	};


	class Demo3 : public DemoBase<Demo3>
	{
	private:
		bool projectToViewport(const glm::mat4& mvp, const glm::vec3& p,
			std::uint32_t width, std::uint32_t height, glm::vec2& out);

		void runMathCheck();
		void runCubeCheck();

	public:
		void runImpl();

	};
}