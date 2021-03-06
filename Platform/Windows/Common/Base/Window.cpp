#include "Window.h"
#include "WindowContainer.h"
#include "Runtime/Debugger/Logger.h"
#pragma comment(lib,"dwmapi.lib")
#pragma warning(disable:4244)
namespace Editor{
	std::unordered_set<BaseWindow*> BaseWindow::mScheduledWindows;
	BaseWindow::BaseWindow() : mParent(nullptr){
		SetRect(0, 0, 200, 200);
		SetMinRect(0, 0, -1, -1);
		mMaxRect = { 0,0,-1,-1 };
		mBKGColor = Gdiplus::Color(41,41,41);
		mLeftNCSize = 6;
		mRightNCSize = 6;
		mTopNCSize = 26;
		mBottomNCSize = 6;
		mbEnableCornerResizing = true;
		mSizingBorderSize = 6;
		mLeftSiblingWindows = nullptr;
		mRightSiblingWindows = nullptr;
		mTopSiblingWindows = nullptr;
		mBottomSiblingWindows = nullptr;
		mParentContainer = nullptr;
		memset(mName, 0, 64);
	}
	BaseWindow::~BaseWindow(){
		DestroyWindow(mhWnd);
	}
	void BaseWindow::SetMinRect(int x, int y, int width, int height){
		mMinRect.X = x;
		mMinRect.Y = y;
		mMinRect.Width = width;
		mMinRect.Height = height;
	}
	void BaseWindow::SetMaxRect(int x, int y, int width, int height) {
		mMaxRect.X = x;
		mMaxRect.Y = y;
		mMaxRect.Width = width;
		mMaxRect.Height = height;
	}
	Gdiplus::Rect & BaseWindow::GetMinRect(){
		return mMinRect;
	}
	int BaseWindow::GetMinWidth(){
		return mMinRect.Width;
	}
	int BaseWindow::GetMinHeight(){
		return mMinRect.Height;
	}
	int BaseWindow::GenerateMinWidth() {
		int min_width = mMinRect.Width;
		int right_sibling_min_width = 0;
		WindowHolder*right_sibling = mRightSiblingWindows;
		while (right_sibling!=nullptr){
			int current_right_sibling_window_min_width = right_sibling->mWindow->GenerateMinWidth();
			if (right_sibling_min_width<current_right_sibling_window_min_width){
				right_sibling_min_width = current_right_sibling_window_min_width;
			}
			right_sibling = right_sibling->Next<WindowHolder>();
		}
		return min_width + right_sibling_min_width;
	}
	int BaseWindow::GenerateMinHeight() {
		int min_height = mMinRect.Height;
		int right_sibling_min_height = 0;
		WindowHolder*bottom_sibling = mBottomSiblingWindows;
		while (bottom_sibling != nullptr) {
			int current_right_sibling_window_min_height = bottom_sibling->mWindow->GenerateMinHeight();
			if (right_sibling_min_height < current_right_sibling_window_min_height) {
				right_sibling_min_height = current_right_sibling_window_min_height;
			}
			bottom_sibling = bottom_sibling->Next<WindowHolder>();
		}
		return min_height + right_sibling_min_height;
	}
	int BaseWindow::GenerateMaxWidth() {
		int max_width = mMaxRect.Width;
		int right_sibling_max_width = 999999;
		WindowHolder*sibling = mRightSiblingWindows;
		while (sibling != nullptr) {
			int current_right_sibling_window_max_width = sibling->mWindow->GenerateMaxWidth();
			if (right_sibling_max_width > current_right_sibling_window_max_width) {
				right_sibling_max_width = current_right_sibling_window_max_width;
			}
			sibling = sibling->Next<WindowHolder>();
		}
		return (max_width == -1 || right_sibling_max_width == -1) ? -1 : max_width + right_sibling_max_width;
	}
	int BaseWindow::GenerateMaxHeight() {
		int max_height = mMaxRect.Height;
		int bottom_sibling_max_height = 999999;
		WindowHolder*sibling = mBottomSiblingWindows;
		while (sibling != nullptr) {
			int current_bottom_sibling_window_max_height = sibling->mWindow->GenerateMinHeight();
			if (bottom_sibling_max_height > current_bottom_sibling_window_max_height) {
				bottom_sibling_max_height = current_bottom_sibling_window_max_height;
			}
			sibling = sibling->Next<WindowHolder>();
		}
		return (max_height==-1||bottom_sibling_max_height==-1) ? -1 : max_height + bottom_sibling_max_height;
	}
	static BaseWindow*sRootWindow=nullptr;
	void BaseWindow::ScheduleUpdate(){
		if (sRootWindow!=this){
			sRootWindow->PushBack(this);
		}
	}
	void BaseWindow::CancelUpdate(){
		sRootWindow->Remove(this);
	}
	void BaseWindow::SetRootWindow(BaseWindow*window){
		sRootWindow = window;
	}
	void BaseWindow::DrawContent(Gdiplus::Graphics&painter){
	}
	void BaseWindow::OnPostPaint() {
		HWND cWnd = GetWindow(mhWnd, GW_CHILD);
		while (cWnd != nullptr) {
			BaseWindow*vw = WindowInstance<BaseWindow>(cWnd);
			InvalidateRect(vw->GetHwnd(), nullptr, true);
			cWnd = GetNextWindow(cWnd, GW_HWNDNEXT);
		}
	}
	void BaseWindow::OnEraseBKG() {
	}
	void BaseWindow::OnPaint(const Gdiplus::Rect & rect_need_update){
		Gdiplus::Graphics painter(mHDC);
		painter.Clear(mBKGColor);
		DrawContent(painter);
	}
	void BaseWindow::OnSize(WPARAM wParam, LPARAM lParam, void*reserved){
		mRect.Width = LOWORD(lParam);
		mRect.Height = HIWORD(lParam);
		HWND cWnd = GetWindow(mhWnd, GW_CHILD);
		while (cWnd != nullptr) {
			BaseWindow*vw = WindowInstance<BaseWindow>(cWnd);
			//vw->OnParentResized(mRect.Width, mRect.Height);
			cWnd = GetNextWindow(cWnd, GW_HWNDNEXT);
		}
	}
	void BaseWindow::OnMoving(WPARAM wParam, LPARAM lParam, void*reserved /* = nullptr */) {
		RECT * rect = (RECT*)lParam;
		mRect.X = rect->left;
		mRect.Y = rect->top;
	}
	void BaseWindow::OnGetMinMaxInfo(WPARAM wParam, LPARAM lParam, void*reserved /* = nullptr */) {
		MINMAXINFO *ptr = (MINMAXINFO*)lParam;
		if (mMinRect.Width!=-1){
			ptr->ptMinTrackSize.x = mMinRect.Width;
		}
		if (mMinRect.Height != -1) {
			ptr->ptMinTrackSize.y = mMinRect.Height;
		}
		if (mMaxRect.Width != -1) {
			ptr->ptMaxTrackSize.x = mMaxRect.Width;
		}
		if (mMaxRect.Height!=-1){
			ptr->ptMaxTrackSize.y = mMaxRect.Height;
		}
	}
	void BaseWindow::OnWindowMove(BaseWindow*movedWindow){
	}
	void BaseWindow::OnEditControlLoseFocus(BaseWindow*editControl){
	}
	LRESULT BaseWindow::OnNCACTIVATE(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, void*reserved /* = nullptr */) {
		return TRUE;
	}
	LRESULT BaseWindow::OnNCHITTEST(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, void*reserved /* = nullptr */) {
		RECT WindowRect;
		GetWindowRect(hWnd, &WindowRect);
		POINT point;
		point.x = LOWORD(lParam) - WindowRect.left;
		point.y = HIWORD(lParam) - WindowRect.top;
		if (mLeftNCSize > 0 && point.x < mSizingBorderSize) {
			if (point.y < mSizingBorderSize && mTopNCSize >0 && mbEnableCornerResizing) {
				return HTTOPLEFT;
			}
			else if (point.y > (mRect.Height - mSizingBorderSize) && mBottomNCSize>0 && mbEnableCornerResizing) {
				return HTBOTTOMLEFT;
			}
			return HTLEFT;
		}
		else if (point.x > mSizingBorderSize && point.x < (mRect.Width - mSizingBorderSize)) {
			if (mTopNCSize > 0 && point.y <= mSizingBorderSize) {
				return HTTOP;
			}
			else if (mBottomNCSize > 0 && point.y >= (mRect.Height - mSizingBorderSize)) {
				return HTBOTTOM;
			}
			return HTCLIENT;
		}
		else if (mRightNCSize > 0 && point.x>mRect.Width-mSizingBorderSize) {
			if (point.y <= mSizingBorderSize && mbEnableCornerResizing) {
				return HTTOPRIGHT;
			}
			else if (point.y >= (mRect.Height - mSizingBorderSize) && mBottomNCSize>0 && mbEnableCornerResizing) {
				return HTBOTTOMRIGHT;
			}
			return HTRIGHT;
		}
		return HTCLIENT;
	}
	LRESULT BaseWindow::OnNCCALCSIZE(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, void*reserved /* = nullptr */) {
		NCCALCSIZE_PARAMS* pParams = NULL;
		RECT* pRect = NULL;
		BOOL bValue = static_cast<BOOL>(wParam);
		if (bValue) {
			pParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
			pRect = &pParams->rgrc[0];
			pRect->left = pRect->left + mLeftNCSize;
			pRect->top = pRect->top + mTopNCSize;
			pRect->right = pRect->right - mRightNCSize;
			pRect->bottom = pRect->bottom - mBottomNCSize;
			pParams->rgrc[1] = pParams->rgrc[0];
		}
		else {
			DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}
	LRESULT BaseWindow::OnNCPAINT(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, void*reserved /* = nullptr */) {
		Debug("BaseWindow::OnNCPAINT %s", mName);
		HDC WindowDC = GetWindowDC(hWnd);
		RECT ClientRect;
		GetClientRect(hWnd, &ClientRect);
		OffsetRect(&ClientRect, mLeftNCSize, mTopNCSize);
		ExcludeClipRect(WindowDC, ClientRect.left, ClientRect.top, ClientRect.right, ClientRect.bottom);
		Gdiplus::Graphics painter(WindowDC);
		painter.Clear(mBKGColor);
		ReleaseDC(hWnd, WindowDC);
		return 0;
	}
	void BaseWindow::Show(){
		if (!IsWindowVisible(mhWnd)) {
			OnShow();
			ShowWindow(mhWnd, SW_SHOW);
			UpdateWindow(mhWnd);
		}
	}
	void BaseWindow::OnShow(){
	}
	void BaseWindow::Hide() {
		if (IsWindowVisible(mhWnd)) {
			OnShow();
			ShowWindow(mhWnd, SW_HIDE);
			UpdateWindow(mhWnd);
		}
	}
	void BaseWindow::OnHide() {
	}
	bool BaseWindow::IsVisiable(){
		return IsWindowVisible(mhWnd);
	}
	void BaseWindow::SetParent(BaseWindow*parent){
		if (parent != nullptr){
			::SetParent(mhWnd, parent->GetHwnd());
		}
		mParent = parent;
	}
	void BaseWindow::SetBkgColor(Gdiplus::Color &color){
		mBKGColor = color;
	}
	void BaseWindow::Update(){
		UpdateWindow(mhWnd);
	}
	void BaseWindow::MarkDirty() {
		InvalidateRect(mhWnd, nullptr, false);
	}
	int BaseWindow::GetX(){
		return mRect.X;
	}
	int BaseWindow::GetY(){
		return mRect.Y;
	}
	int BaseWindow::GetWidth(){
		return mRect.Width;
	}
	int BaseWindow::GetHeight(){
		return mRect.Height;
	}
	void BaseWindow::SetRect(int x, int y, int width, int height){
		mRect.X = x;
		mRect.Y = y;
		mRect.Width = width;
		mRect.Height = height;
	}
	void BaseWindow::SetRect(Gdiplus::Rect &rect){
		mRect = rect;
	}
	Gdiplus::Rect BaseWindow::GetRect(){
		return mRect;
	}
	void BaseWindow::MoveWindow(int x, int y, int width, int height){
		::MoveWindow(mhWnd, x, y, width, height,true);
	}
	void BaseWindow::SetSize(int width, int height,HWND param){
		mRect.Width = width;
		mRect.Height = height;
		SetWindowPos(mhWnd, param, 0, 0, mRect.Width, mRect.Height, SWP_SHOWWINDOW);
	}
	void BaseWindow::SetNCSize(int left, int right, int bottom, int top) {
		mTopNCSize = top;
		mBottomNCSize = bottom;
		mLeftNCSize = left;
		mRightNCSize = right;
	}
	void BaseWindow::EnableCornerResizing(bool enable) {
		mbEnableCornerResizing = enable;
	}
	void BaseWindow::SetSizingBorderSize(int size) {
		mSizingBorderSize = size;
	}
	BaseWindow*BaseWindow::GetParent(){
		return mParent;
	}
	HWND BaseWindow::GetHwnd(){
		return mhWnd;
	}
	const char*BaseWindow::GetWindowName(){
		return mName;
	}
	void BaseWindow::SetWindowName(const char*name){
		memset(mName, 0, 64);
		strcpy(mName, name);
	}
	void BaseWindow::GetRelativeWindowRect(Gdiplus::Rect &rect) {
		RECT self_window_rect;
		GetWindowRect(mhWnd, &self_window_rect);
		POINT pos = {self_window_rect.left,self_window_rect.top};
		ScreenToClient(mParent->GetHwnd(),&pos);
		rect.X = pos.x;
		rect.Y = pos.y;
		rect.Width = self_window_rect.right - self_window_rect.left;
		rect.Height = self_window_rect.bottom - self_window_rect.top;
	}
	int BaseWindow::GetUILocationL(int size, int distance) {
		return distance + size / 2;
	}
	int BaseWindow::GetUILocationR(int size, int distance) {
		return mRect.Width - distance - size / 2;
	}
	int BaseWindow::GetUILocationT(int size, int distance) {
		return distance + size / 2;
	}
	int BaseWindow::GetUILocationB(int size, int distance) {
		return mRect.Height - distance - size / 2;
	}
	HDC BaseWindow::GetDC(){
		return mHDC;
	}
	void BaseWindow::ExtentWindowFromLeft(int & deltaX, const Gdiplus::Rect * left_rect, const Gdiplus::Rect * container_rect) {
		GetRelativeWindowRect(mRect);
		const Gdiplus::Rect * reference_rect = left_rect == nullptr ? container_rect : left_rect;
		int left_rect_right_edge_pos = (reference_rect == nullptr) ? mRect.X : (reference_rect == container_rect ? reference_rect->X : reference_rect->GetRight());
		mPredefinedRect = { left_rect_right_edge_pos,mRect.Y,mRect.Width + deltaX,mRect.Height };
		deltaX = 0;
		MoveWindow(mPredefinedRect.X, mPredefinedRect.Y, mPredefinedRect.Width, mPredefinedRect.Height);
		WindowHolder*right_sibling = mRightSiblingWindows;
		while (right_sibling != nullptr) {
			right_sibling->mWindow->ExtentWindowFromLeft(deltaX, &mPredefinedRect);
			right_sibling = right_sibling->Next<WindowHolder>();
		}
	}
	void BaseWindow::ReduceWindowFromLeft(int & deltaX, const Gdiplus::Rect * left_rect, const Gdiplus::Rect * container_rect) {
		GetRelativeWindowRect(mRect);
		const Gdiplus::Rect * reference_rect = left_rect == nullptr ? container_rect : left_rect;
		int left_rect_right_edge_pos = (reference_rect == nullptr) ? mRect.X : (reference_rect == container_rect ? reference_rect->X : reference_rect->GetRight());
		int max_cosumable_deltaX = mMinRect.Width==-1? 32 - mRect.Width : mMinRect.Width - mRect.Width;
		int current_window_cosumed_deltaX = deltaX > max_cosumable_deltaX ? deltaX : max_cosumable_deltaX;
		deltaX -= current_window_cosumed_deltaX;
		mPredefinedRect = { left_rect_right_edge_pos,mRect.Y,mRect.Width + current_window_cosumed_deltaX,mRect.Height };
		MoveWindow(mPredefinedRect.X, mPredefinedRect.Y, mPredefinedRect.Width, mPredefinedRect.Height);
		WindowHolder*sibling = mRightSiblingWindows;
		while (sibling != nullptr) {
			sibling->mWindow->ExtentWindowFromLeft(deltaX, &mPredefinedRect);
			sibling = sibling->Next<WindowHolder>();
		}
	}
	void BaseWindow::ExtentWindowFromRight(int & deltaX, const Gdiplus::Rect * right_rect) {
		GetRelativeWindowRect(mRect);
		mPredefinedRect = { mRect.X,mRect.Y,mRect.Width + deltaX,mRect.Height };
		deltaX = 0;
		MoveWindow(mPredefinedRect.X, mPredefinedRect.Y, mPredefinedRect.Width, mPredefinedRect.Height);
	}
	void BaseWindow::ReduceWindowFromRight(int & deltaX, int shiftX, const Gdiplus::Rect * container_rect) {
		GetRelativeWindowRect(mRect);
		int max_cosumable_deltaX = mMinRect.Width==-1 ? 32-mRect.Width : mMinRect.Width - mRect.Width;
		int current_window_cosumed_deltaX = deltaX > max_cosumable_deltaX  ? deltaX : max_cosumable_deltaX;
		deltaX -= current_window_cosumed_deltaX;
		mPredefinedRect = { mRect.X + deltaX + shiftX ,mRect.Y,mRect.Width + current_window_cosumed_deltaX,mRect.Height };
		MoveWindow(mPredefinedRect.X, mPredefinedRect.Y, mPredefinedRect.Width, mPredefinedRect.Height);
		WindowHolder*sibling = mLeftSiblingWindows;
		while (sibling != nullptr) {
			sibling->mWindow->ReduceWindowFromRight(deltaX, shiftX,&mPredefinedRect);
			sibling = sibling->Next<WindowHolder>();
		}
	}
	void BaseWindow::ExtentWindowFromBottom(int & deltaY, const Gdiplus::Rect * bottom_rect) {
		GetRelativeWindowRect(mRect);
		mPredefinedRect = { mRect.X,mRect.Y,mRect.Width ,mRect.Height + deltaY };
		deltaY = 0;
		MoveWindow(mPredefinedRect.X, mPredefinedRect.Y, mPredefinedRect.Width, mPredefinedRect.Height);
	}
	void BaseWindow::ReduceWindowFromBottom(int & deltaY, int shiftY, const Gdiplus::Rect * container_rect) {
		GetRelativeWindowRect(mRect);
		int max_cosumable_deltaY = mMinRect.Height== -1 ? 32 - mRect.Height : mMinRect.Height - mRect.Height;
		int current_window_cosumed_deltaY = deltaY > max_cosumable_deltaY ? deltaY : max_cosumable_deltaY;
		deltaY -= current_window_cosumed_deltaY;
		mPredefinedRect = { mRect.X  ,mRect.Y + deltaY + shiftY,mRect.Width ,mRect.Height + current_window_cosumed_deltaY };
		MoveWindow(mPredefinedRect.X, mPredefinedRect.Y, mPredefinedRect.Width, mPredefinedRect.Height);
		WindowHolder*sibling = mTopSiblingWindows;
		while (sibling != nullptr) {
			sibling->mWindow->ReduceWindowFromBottom(deltaY, shiftY, &mPredefinedRect);
			sibling = sibling->Next<WindowHolder>();
		}
	}
	void BaseWindow::ExtentWindowFromTop(int & deltaY, const Gdiplus::Rect * top_rect, const Gdiplus::Rect * container_rect) {
		GetRelativeWindowRect(mRect);
		const Gdiplus::Rect * reference_rect = top_rect == nullptr ? container_rect : top_rect;
		int top_rect_bottom_edge_pos = (reference_rect == nullptr) ? mRect.Y : (reference_rect == container_rect ? reference_rect->Y : reference_rect->GetBottom());
		mPredefinedRect = { mRect.X,top_rect_bottom_edge_pos,mRect.Width,mRect.Height+deltaY };
		deltaY = 0;
		MoveWindow(mPredefinedRect.X, mPredefinedRect.Y, mPredefinedRect.Width, mPredefinedRect.Height);
		WindowHolder*sibling = mBottomSiblingWindows;
		while (sibling != nullptr) {
			sibling->mWindow->ExtentWindowFromTop(deltaY, &mPredefinedRect);
			sibling = sibling->Next<WindowHolder>();
		}
	}
	void BaseWindow::ReduceWindowFromTop(int & deltaY, const Gdiplus::Rect * top_rect, const Gdiplus::Rect * container_rect) {
		GetRelativeWindowRect(mRect);
		const Gdiplus::Rect * reference_rect = top_rect == nullptr ? container_rect : top_rect;
		int top_rect_bottom_edge_pos = (reference_rect == nullptr) ? mRect.Y : (reference_rect == container_rect ? reference_rect->Y : reference_rect->GetBottom());
		int max_cosumable_deltaY = mMinRect.Height - mRect.Height;
		int current_window_cosumed_deltaY= deltaY > max_cosumable_deltaY ? deltaY : max_cosumable_deltaY;
		deltaY -= current_window_cosumed_deltaY;
		mPredefinedRect = { mRect.X,top_rect_bottom_edge_pos,mRect.Width ,mRect.Height + current_window_cosumed_deltaY };
		MoveWindow(mPredefinedRect.X, mPredefinedRect.Y, mPredefinedRect.Width, mPredefinedRect.Height);
		WindowHolder*sibling = mBottomSiblingWindows;
		while (sibling != nullptr) {
			sibling->mWindow->ReduceWindowFromTop(deltaY, &mPredefinedRect);
			sibling = sibling->Next<WindowHolder>();
		}
	}
	ATOM BaseWindow::RegisterWindowClass(UINT style, LPCTSTR pWndClassName, WNDPROC wndProc){
		WNDCLASSEX wcex;
		memset(&wcex, 0, sizeof(wcex));
		wcex.cbSize = sizeof(wcex);
		wcex.style = style;
		wcex.lpfnWndProc = wndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = GetModuleHandle(NULL);
		wcex.hIcon = NULL;
		wcex.hIconSm = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = pWndClassName;
		ATOM atom = RegisterClassEx(&wcex);
		if (!atom){
			return 0;
		}
		return atom;
	}
}