#include "D3DFramework.h"

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nShowCmd) {

	auto& render = D3DFramework::getInstance();

	if (FAILED(render.initWindow(hInstance, nShowCmd)))
		return 0;

	if (FAILED(render.initDevice()))
		return 0;

	// Main message loop
	MSG msg;
	msg.message = 0;
	while (WM_QUIT != msg.message) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			render.render();
		}
	}

	return static_cast<int>(msg.wParam);
}