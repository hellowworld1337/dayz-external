#include <array>

constexpr auto OverlayCaption = L"NVIDIA";
constexpr auto OverlayClassName = L"NVIDIA";

enum TextLayout : std::uint32_t
{
	TextLeft = 0x00,
	TextRight = 0x01,
	TextCenterV = 0x02,
	TextCenterH = 0x04,
	TextCenter = (TextCenterV | TextCenterH),
};

class COverlayProxy
{
public:
	ImFont* m_pFont = nullptr;
	void Line(const ImVec2& begin, const ImVec2& end, const ImColor& color);
	void RectFilled(const ImVec2& location, const ImVec2& size, const ImColor& color);
	void Rect(const ImVec2& location, const ImVec2& size, const ImColor& color);
	void Text(const ImVec2& location, const std::uint32_t layout, const ImColor& color, const char* text, ...);
	inline ImDrawList* GetDrawList()
	{
		return m_render_context;
	}
protected:
	void RenderText(const ImVec2& location, const std::uint32_t layout, const ImColor& color, const char* text);
	ImDrawList* m_render_context = nullptr;
};

void COverlayProxy::Line(const ImVec2& begin, const ImVec2& end, const ImColor& color)
{
	m_render_context->AddLine(begin, end, color);
}

void COverlayProxy::RectFilled(const ImVec2& location, const ImVec2& size, const ImColor& color)
{
	m_render_context->AddRectFilled(location, { location.x + size.x, location.y + size.y }, color, 0.f, ImDrawCornerFlags_All);
}

void COverlayProxy::Rect(const ImVec2& location, const ImVec2& size, const ImColor& color)
{
	m_render_context->AddRect(location, { location.x + size.x, location.y + size.y }, color, 0.f, ImDrawCornerFlags_All);
}

void COverlayProxy::Text(const ImVec2& location, const std::uint32_t layout, const ImColor& color, const char* text, ...)
{
	char format[2048] = { };

	va_list args = { };
	va_start(args, text);
	vsprintf_s(format, text, args);
	va_end(args);

	RenderText(location, layout, color, format);
}

void COverlayProxy::RenderText(const ImVec2& location, const std::uint32_t layout, const ImColor& color, const char* text)
{
	auto text_size = ImGui::CalcTextSize(text);
	auto text_location = location;

	if (layout & TextRight)
		text_location.x -= text_size.x;
	else if (layout & TextCenterH)
		text_location.x -= text_size.x / 2.f;

	if (layout & TextCenterV)
		text_location.y -= text_size.y / 2.f;

	auto text_shadow = ImVec2(text_location.x + 1.f, text_location.y + 1.f);

	m_render_context->AddText(text_shadow, ImColor{ 0.01f, 0.01f, 0.01f, color.Value.w }, text);
	m_render_context->AddText(text_location, color, text);
}


class Overlay : public COverlayProxy
{
public:
	using Shared = std::shared_ptr< Overlay >;
	using Dimension = std::array< std::int32_t, 4 >;
	using UpdateProcedureFn = void(*)();
	using RenderProcedureFn = void(*)(Overlay* pOverlay, void * pData);
	std::int32_t m_width = 640;
	std::int32_t m_height = 480;
public:
	Overlay();
	Overlay(const std::wstring& target_class_name, bool WaitForWindow);

public:
	~Overlay();

public:
	bool Create(const std::wstring& target_class_name, bool WaitForWindow);
	void Destroy();

public:
	IDXGISwapChain* GetSwapChain() const;
	ID3D11Device* GetDevice() const;
	ID3D11DeviceContext* GetDeviceContext() const;
	HWND GetTargetWindow() const;

public:
	bool Render(bool bTopMostCheck, void * pData = nullptr);

	void SetUpdateProcedure(const UpdateProcedureFn update_procedure);
	void SetRenderProcedure(const RenderProcedureFn render_procedure);

protected:
	void Scale();

public:

protected:
	

protected:
	bool CreateInternal();
	void DestroyInternal();

	bool CreateD3D11();
	void DestroyD3D11();

	bool CreateRenderTarget();
	void DestroyRenderTarget();
protected:
	static LRESULT WINAPI Procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

protected:
	HWND m_window_target = nullptr;
	HWND m_window_overlay = nullptr;

	std::wstring m_overlay_caption = OverlayCaption;
	std::wstring m_overlay_class_name = OverlayClassName;

	IDXGISwapChain* m_swap_chain = nullptr;

	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_device_context = nullptr;

	ID3D11RenderTargetView* m_render_target_view = nullptr;

	UpdateProcedureFn m_update_procedure = nullptr;
	RenderProcedureFn m_render_procedure = nullptr;
	//ImDrawList* m_render_context = nullptr;
};

Overlay::Shared pOverlay = { };


Overlay::Dimension GetWindowDimension(HWND window)
{
	Overlay::Dimension dimension = { };

	if (window)
	{
		RECT rect_client = { };
		GetClientRect(window, &rect_client);

		dimension[2] = rect_client.right;
		dimension[3] = rect_client.bottom;

		RECT rect_window = { };
		GetWindowRect(window, &rect_window);

		POINT distance = { 0, 0 };
		ClientToScreen(window, &distance);

		dimension[0] = rect_window.left + (distance.x - rect_window.left);
		dimension[1] = rect_window.top + (distance.y - rect_window.top);
	}

	return dimension;
}

Overlay::Overlay()
	: m_overlay_caption(OverlayCaption)
	, m_overlay_class_name(OverlayClassName)
	, m_width(640)
	, m_height(480)
{ }

Overlay::Overlay(const std::wstring& target_class_name, bool WaitForWindow)
	: Overlay()
{
	if (!Create(target_class_name, WaitForWindow))
		TRACE("%s: Create() error!", __FUNCTION__);
}

Overlay::~Overlay()
{
	Destroy();
}

ImWchar* BuildFontGlyphRanges()
{
	static ImVector<ImWchar> ranges;
	if (ranges.empty())
	{
		ImFontGlyphRangesBuilder builder;
		constexpr ImWchar baseRanges[]
		{
			0x0100, 0x024F, // Latin Extended-A + Latin Extended-B
			0x0300, 0x03FF, // Combining Diacritical Marks + Greek/Coptic
			0x0600, 0x06FF, // Arabic
			0x0E00, 0x0E7F, // Thai
			0
		};
		builder.AddRanges(baseRanges);
		builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
		builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
		builder.AddText("\u9F8D\u738B\u2122");
		builder.BuildRanges(&ranges);
	}
	return ranges.Data;
}

bool Overlay::Create(const std::wstring& target_class_name, bool WaitForWindow)
{
	if (target_class_name.empty())
	{
		TRACE("%s: target_class_name is empty!", __FUNCTION__);
		return false;
	}

	do
	{
		m_window_target = FindWindowW(target_class_name.c_str(), nullptr);

	} while (m_window_target == nullptr && WaitForWindow);

	if (!m_window_target)
	{
		TRACE("%s: FindWindowW() error! (0x%08X)", __FUNCTION__, GetLastError());
		return false;
	}

	if (!CreateInternal())
	{
		TRACE("%s: CreateInternal() error!", __FUNCTION__);
		return false;
	}

	if (!CreateD3D11())
	{
		TRACE("%s: CreateD3D11() error!", __FUNCTION__);
		return false;
	}

	if (!CreateRenderTarget())
	{
		TRACE("%s: CreateRenderTarget() error!", __FUNCTION__);
		return false;
	}

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	if (!ImGui_ImplWin32_Init(m_window_overlay))
	{
		TRACE("%s: ImGui_ImplWin32_Init() error!", __FUNCTION__);
		return false;
	}

	if (!ImGui_ImplDX11_Init(m_device, m_device_context))
	{
		TRACE("%s: ImGui_ImplDX11_Init() error!", __FUNCTION__);
		return false;
	}

	PWSTR PathToFonts = {};

	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &PathToFonts)))
	{
		std::string szString = Core::WideToMultibyte(std::wstring(PathToFonts));
		CoTaskMemFree(PathToFonts);

		ImFontConfig cfg;
		cfg.OversampleV = 3;

		auto FullString = szString.append("\\segoeui.ttf");
		printf("[FACE] Full font path: %s\n", FullString.c_str());
		this->m_pFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(FullString.c_str(), 15.0f, &cfg, BuildFontGlyphRanges());
	}

	m_render_context = ImGui::GetOverlayDrawList();

	return true;
}

void Overlay::Destroy()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	DestroyRenderTarget();
	DestroyD3D11();
	DestroyInternal();

	m_window_target = nullptr;
	m_window_overlay = nullptr;
	m_overlay_caption.clear();
	m_overlay_class_name.clear();
	m_swap_chain = nullptr;
	m_device = nullptr;
	m_device_context = nullptr;
	m_render_target_view = nullptr;
}

IDXGISwapChain* Overlay::GetSwapChain() const
{
	return m_swap_chain;
}

ID3D11Device* Overlay::GetDevice() const
{
	return m_device;
}

ID3D11DeviceContext* Overlay::GetDeviceContext() const
{
	return m_device_context;
}

inline HWND Overlay::GetTargetWindow() const
{
	return m_window_target;
}

bool Overlay::Render(bool bTopMostCheck, void* pData)
{
	static const ImVec4 color_clear = { 0.f, 0.f, 0.f, 0.f };

	if (m_update_procedure)
		m_update_procedure();

	MSG msg = { };

	if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			return false;

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	// Scale();

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::PushFont(this->m_pFont);

	bool bCanDraw = bTopMostCheck ? GetForegroundWindow() == m_window_target : true;

	if(bCanDraw)
	{
		if (m_render_procedure)
			m_render_procedure(this, pData);
	}

	ImGui::PopFont();

	ImGui::Render();
	m_device_context->OMSetRenderTargets(1, &m_render_target_view, nullptr);
	m_device_context->ClearRenderTargetView(m_render_target_view, reinterpret_cast<const FLOAT*>(&color_clear));
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	auto result = m_swap_chain->Present(0, 0);

	if (FAILED(result))
	{
		TRACE("%s: m_swap_chain->Present() error! (0x%08X)", __FUNCTION__, result);
		return false;
	}

	return true;
}

void Overlay::SetUpdateProcedure(const UpdateProcedureFn update_procedure)
{
	if (!update_procedure)
		return;

	m_update_procedure = update_procedure;
}

void Overlay::SetRenderProcedure(const RenderProcedureFn render_procedure)
{
	if (!render_procedure)
		return;

	m_render_procedure = render_procedure;
}

void Overlay::Scale()
{
	auto scale = [](std::int32_t& input, std::int32_t& output)
	{
		if (input == 0)
		{
			input--;
			output++;
		}
	};

	auto dimension = GetWindowDimension(m_window_target);

	m_width = dimension[2];
	m_height = dimension[3];

	scale(dimension[0], m_width);
	scale(dimension[1], m_height);

	MoveWindow(m_window_overlay, dimension[0], dimension[1], m_width, m_height, TRUE);
}

bool Overlay::CreateInternal()
{
	WNDCLASSEXW window_class =
	{
		sizeof(WNDCLASSEXW),
		0,
		Procedure,
		0,
		0,
		nullptr,
		LoadIconW(nullptr, IDI_APPLICATION),
		LoadCursorW(nullptr, IDC_ARROW),
		nullptr,
		nullptr,
		m_overlay_class_name.c_str(),
		LoadIconW(nullptr, IDI_APPLICATION),
	};

	if (!RegisterClassEx(&window_class))
	{
		TRACE("%s: RegisterClassEx() error! (0x%08X)", __FUNCTION__, GetLastError());
		return false;
	}

	m_window_overlay = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
		m_overlay_class_name.c_str(), m_overlay_caption.c_str(),
		WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
		nullptr, nullptr, nullptr, nullptr);

	if (!m_window_overlay)
	{
		TRACE("%s: CreateWindowExW() error! (0x%08X)", __FUNCTION__, GetLastError());
		return false;
	}

	if (!SetLayeredWindowAttributes(m_window_overlay, RGB(0, 0, 0), 255, ULW_COLORKEY | LWA_ALPHA))
	{
		TRACE("%s: SetLayeredWindowAttributes() error! (0x%08X)", __FUNCTION__, GetLastError());
		return false;
	}

	auto dimension = GetWindowDimension(m_window_target);

	const auto result = DwmExtendFrameIntoClientArea(m_window_overlay, reinterpret_cast<const MARGINS*>(&dimension));

	if (FAILED(result))
	{
		TRACE("%s: DwmExtendFrameIntoClientArea() error! (0x%08X)", __FUNCTION__, result);
		return false;
	}

	ShowWindow(m_window_overlay, SW_SHOWDEFAULT);

	if (!UpdateWindow(m_window_overlay))
	{
		TRACE("%s: UpdateWindow() error! (0x%08X)", __FUNCTION__, GetLastError());
		return false;
	}

	Scale();
	return true;
}

void Overlay::DestroyInternal()
{
	UnregisterClassW(m_overlay_class_name.c_str(), nullptr);

	if (m_window_overlay)
		DestroyWindow(m_window_overlay);
}

bool Overlay::CreateD3D11()
{
	DXGI_SWAP_CHAIN_DESC swap_chain_desc = { };

	swap_chain_desc.BufferCount = 2;
	swap_chain_desc.BufferDesc.Width = 0;
	swap_chain_desc.BufferDesc.Height = 0;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.OutputWindow = m_window_overlay;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.Windowed = TRUE;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	const D3D_FEATURE_LEVEL feature_level_array[2] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

	const auto result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, feature_level_array, 2, D3D11_SDK_VERSION,
		&swap_chain_desc, &m_swap_chain, &m_device, &feature_level, &m_device_context);

	if (FAILED(result))
	{
		TRACE("%s: D3D11CreateDeviceAndSwapChain() error! (0x%08X)", __FUNCTION__, result);
		return false;
	}

	return true;
}

void Overlay::DestroyD3D11()
{
	if (m_device_context)
		m_device_context->Release();

	if (m_device)
		m_device->Release();

	if (m_swap_chain)
		m_swap_chain->Release();
}

bool Overlay::CreateRenderTarget()
{
	ID3D11Texture2D* texture2d = nullptr;

	auto result = m_swap_chain->GetBuffer(0, IID_PPV_ARGS(&texture2d));

	if (FAILED(result))
	{
		TRACE("%s: m_swap_chain->GetBuffer() error! (0x%08X)", __FUNCTION__, result);
		return false;
	}

	result = m_device->CreateRenderTargetView(texture2d, nullptr, &m_render_target_view);

	texture2d->Release();

	if (FAILED(result))
	{
		TRACE("%s: m_device->CreateRenderTargetView() error! (0x%08X)", __FUNCTION__, result);
		return false;
	}

	return true;
}

void Overlay::DestroyRenderTarget()
{
	if (m_render_target_view)
		m_render_target_view->Release();
}

LRESULT Overlay::Procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(EXIT_SUCCESS);
		return 0;
	case WM_KEYDOWN:
		return 0;
	case WM_ERASEBKGND:
		SendMessage(window, WM_PAINT, 0, 0);
		return TRUE;
	case WM_PAINT:
		return 0;
	case WM_SIZE:
		if (pOverlay && pOverlay->GetDevice() != nullptr && wparam != SIZE_MINIMIZED)
		{
			pOverlay->DestroyRenderTarget();
			pOverlay->GetSwapChain()->ResizeBuffers(0, (UINT)LOWORD(lparam), (UINT)HIWORD(lparam), DXGI_FORMAT_UNKNOWN, 0);
			pOverlay->CreateRenderTarget();
		}
		return 0;
	default:
		break;
	}

	return DefWindowProcW(window, message, wparam, lparam);
}