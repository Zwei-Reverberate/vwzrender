#include <demo/demo3.h>
#include <demo/demo2.h>
#include <format>
#include <log.h>
#include <chrono>
#include <Windows.h>
#include <zwindow.h>

std::string demo::formatVec3(const glm::vec3& v)
{
	return std::format("({:.4f}, {:.4f}, {:.4f})", v.x, v.y, v.z);
}

std::string demo::formatVec4(const glm::vec4& v)
{
	return std::format("({:.4f}, {:.4f}, {:.4f}, {:.4f})", v.x, v.y, v.z, v.w);
}

glm::vec3 demo::transformPoint(const glm::mat4& m, const glm::vec3& p)
{
	return glm::vec3(m * glm::vec4(p, 1.0f));
}

void demo::MathCheck::record(std::string_view name, bool ok, const std::string& detail)
{
	++total;
	passed += ok ? 1u : 0u;
	VWZ_LOG_INFO("[数学验证] {:02} {} | {} | {}", total, name, detail, ok ? "通过" : "失败 <--");
}

void demo::MathCheck::number(std::string_view name, float actual, float expected)
{
	record(name, std::abs(actual - expected) <= kEps,
		std::format("= {:.4f},期望 {:.4f}", actual, expected));
}

void demo::MathCheck::vec3(std::string_view name, const glm::vec3& actual, const glm::vec3& expected)
{
	record(name, glm::all(glm::epsilonEqual(actual, expected, kEps)),
		std::format("= {},期望 {}", formatVec3(actual), formatVec3(expected)));
}

void demo::MathCheck::vec4(std::string_view name, const glm::vec4& actual, const glm::vec4& expected)
{
	record(name, glm::all(glm::epsilonEqual(actual, expected, kEps)),
		std::format("= {},期望 {}", formatVec4(actual), formatVec4(expected)));
}

void demo::Demo3::runMathCheck()
{
	VWZ_LOG_INFO("");
	VWZ_LOG_INFO("---- 数学验证(第 3 课):手算对照 ----");
	MathCheck check;

	const glm::vec3 a(1.0f, 2.0f, 3.0f);
	const glm::vec3 b(4.0f, -5.0f, 6.0f);
	check.number("length(3,4,0) = 5", glm::length(glm::vec3(3.0f, 4.0f, 0.0f)), 5.0f);
	check.number("dot((1,2,3),(4,-5,6)) = 12", glm::dot(a, b), 12.0f);
	check.vec3("cross((1,0,0),(0,1,0)) = (0,0,1)",
		glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::vec3(0.0f, 0.0f, 1.0f));
	check.vec3("normalize(0,3,4) = (0,0.6,0.8)", glm::normalize(glm::vec3(0.0f, 3.0f, 4.0f)),
		glm::vec3(0.0f, 0.6f, 0.8f));

	// 旋转与复合(列向量约定:v' = Mv,先做的变换写在右边)
	const glm::mat4 rotZ90 = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	const glm::mat4 rotY90 = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::mat4 moveT = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 0.0f));
	check.vec3("Rz(90) * (1,0,0) = (0,1,0)", transformPoint(rotZ90, glm::vec3(1.0f, 0.0f, 0.0f)),
		glm::vec3(0.0f, 1.0f, 0.0f));
	check.vec3("Ry(90) * (1,0,0) = (0,0,-1)", transformPoint(rotY90, glm::vec3(1.0f, 0.0f, 0.0f)),
		glm::vec3(0.0f, 0.0f, -1.0f));
	check.vec3("T(1,2,0) * Rz(90):先转后移 (1,0,0) -> (1,3,0)",
		transformPoint(moveT * rotZ90, glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(1.0f, 3.0f, 0.0f));
	check.vec3("Rz(90) * T(1,2,0):先移后转 (1,0,0) -> (-2,2,0)",
		transformPoint(rotZ90 * moveT, glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(-2.0f, 2.0f, 0.0f));

	// 齐次坐标:平移只作用于点 (w = 1),不影响方向 (w = 0)
	const glm::mat4 moveTo = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 20.0f, 30.0f));
	check.vec4("T(10,20,30) 作用于点 (1,2,3,1)", moveTo * glm::vec4(1.0f, 2.0f, 3.0f, 1.0f),
		glm::vec4(11.0f, 22.0f, 33.0f, 1.0f));
	check.vec4("T(10,20,30) 作用于方向 (1,2,3,0)", moveTo * glm::vec4(1.0f, 2.0f, 3.0f, 0.0f),
		glm::vec4(1.0f, 2.0f, 3.0f, 0.0f));

	// 相机与投影
	const glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));
	check.vec4("lookAt:世界原点在观察空间 = (0,0,-3,1)", view * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
		glm::vec4(0.0f, 0.0f, -3.0f, 1.0f));

	const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 1.0f, 100.0f);
	const auto ndc = [&proj](const glm::vec4& p)
		{
			const glm::vec4 clip = proj * p;
			return glm::vec3(clip) / clip.w;
		};
	check.number("近平面 (0,0,-1) 的 NDC z = 0", ndc(glm::vec4(0.0f, 0.0f, -1.0f, 1.0f)).z, 0.0f);
	check.number("远平面 (0,0,-100) 的 NDC z = 1", ndc(glm::vec4(0.0f, 0.0f, -100.0f, 1.0f)).z, 1.0f);
	check.number("(1,0,-1) 的 NDC x = 1", ndc(glm::vec4(1.0f, 0.0f, -1.0f, 1.0f)).x, 1.0f);
	check.number("(1,0,-2) 的 NDC x = 0.5(近大远小)", ndc(glm::vec4(1.0f, 0.0f, -2.0f, 1.0f)).x, 0.5f);

	glm::mat4 projFlipY = proj;
	projFlipY[1][1] *= -1.0f;
	const glm::vec4 flipClip = projFlipY * glm::vec4(0.0f, 1.0f, -1.0f, 1.0f);
	check.number("Y 翻转后 (0,1,-1) 的 NDC y = -1(Vulkan 风格)", (glm::vec3(flipClip) / flipClip.w).y, -1.0f);

	const bool allPassed = (check.passed == check.total);
	VWZ_LOG_INFO("MATH-CHECK SUMMARY: {}/{} passed", check.passed, check.total);
	if (allPassed)
	{
		VWZ_LOG_INFO("数学验证全部通过。");
	}
	else
	{
		VWZ_LOG_ERROR("数学验证有 {} 项失败。", check.total - check.passed);
	}
}

bool demo::Demo3::projectToViewport(const glm::mat4& mvp, const glm::vec3& p,
	std::uint32_t width, std::uint32_t height, glm::vec2& out)
{
	const glm::vec4 clip = mvp * glm::vec4(p, 1.0f);
	if (islessequal(clip.w, 1e-4f))
	{
		return false;
	}
	const glm::vec3 ndc = glm::vec3(clip) / clip.w; // 透视除法
	out = glm::vec2((ndc.x * 0.5f + 0.5f) * static_cast<float>(width),
		(ndc.y * 0.5f + 0.5f) * static_cast<float>(height));

	return true;
}

void demo::Demo3::runCubeCheck()
{
	zwin::Application app{ 0, nullptr };

	auto window = createDemoWindow("ZWindow — 第 3 课:向量、矩阵与齐次坐标");
	if (window == nullptr) 
	{
		return;
	}

	// GDI 资源创建
	const HWND hwnd = static_cast<HWND>(window->nativeWindow().window);
	const HDC dc = ::GetDC(hwnd);
	const HBRUSH background = ::CreateSolidBrush(RGB(24, 24, 32));
	const HPEN pen = ::CreatePen(PS_SOLID, 2, RGB(120, 220, 255));

	const auto startTime = std::chrono::steady_clock::now();
	std::uint32_t frameCount = 0;
	VWZ_LOG_INFO("开始渲染:{} 顶点 / {} 条棱线;ESC 或标题栏 X 退出", kCubeVertices.size(), kCubeEdges.size());

	while (window->processEvents()) 
	{
		const std::uint32_t width = window->clientWidth();
		const std::uint32_t height = window->clientHeight();
		if (width == 0 || height == 0) 
		{
			::Sleep(16);
			continue;
		}

		const float seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();

		// 模型矩阵:绕 Y 轴匀速旋转,再固定前倾 20°
		const glm::mat4 model = glm::rotate(glm::mat4(1.0f), seconds * 0.9f, glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		// 观察矩阵:相机位于 (0,0,3) 看向原点(右手系,沿 -Z 方向看)
		const glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
		// 投影矩阵:45° 视场宽;(1,1) 取负把 NDC 的 Y 轴翻成向下,与 Vulkan 约定对齐
		glm::mat4 proj = glm::perspective(glm::radians(45.0f),
			static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
		proj[1][1] *= -1.0f;
		const glm::mat4 mvp = proj * view * model;

		const RECT clientRect{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
		::FillRect(dc, &clientRect, background);
		const HGDIOBJ oldPen = ::SelectObject(dc, pen);

		std::array<glm::vec2, 8> screen{};
		std::array<bool, 8> visible{};
		for (std::size_t i = 0; i < kCubeVertices.size(); ++i) 
		{
			visible[i] = projectToViewport(mvp, kCubeVertices[i], width, height, screen[i]);
		}
		for (const CubeEdge& edge : kCubeEdges) 
		{
			if (!visible[edge.a] || !visible[edge.b]) 
			{
				continue; // 相机后方的点:跳过该棱
			}
			::MoveToEx(dc, static_cast<int>(std::lround(screen[edge.a].x)),
				static_cast<int>(std::lround(screen[edge.a].y)), nullptr);
			::LineTo(dc, static_cast<int>(std::lround(screen[edge.b].x)),
				static_cast<int>(std::lround(screen[edge.b].y)));
		}
		::SelectObject(dc, oldPen);

		++frameCount;
		if (frameCount == 1) 
		{
			VWZ_LOG_INFO("首帧完成:客户区 {}x{}", width, height);
		}
		else if (frameCount % 240 == 0) 
		{
			VWZ_LOG_INFO("[演示] 第 {} 帧,旋转角 {:.0f}°,平均 {:.0f} FPS", frameCount,
				glm::degrees(seconds * 0.9f), frameCount / seconds);
		}
		::Sleep(16); // ~60 FPS 节流
	}

	::DeleteObject(pen);
	::DeleteObject(background);
	::ReleaseDC(hwnd, dc);

	VWZ_LOG_INFO("主循环退出,共渲染 {} 帧,销毁窗口", frameCount);
}

void demo::Demo3::runImpl()
{
	runMathCheck();
	runCubeCheck();
}