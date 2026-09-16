#pragma once
#include <Windows.h>
#include <string>
#include "Types.hpp"

inline HWND   g_overlay = nullptr;
inline HDC    g_hdc     = nullptr;
inline HDC    g_memDC   = nullptr;
inline HBITMAP g_memBmp = nullptr;
inline HBITMAP g_oldBmp = nullptr;
inline int    g_width   = 1920;
inline int    g_height  = 1080;

inline LRESULT CALLBACK OverlayProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) { PAINTSTRUCT ps; BeginPaint(h, &ps); EndPaint(h, &ps); }
    return DefWindowProcW(h, m, w, l);
}

inline void RebuildMemDC() {
    if (g_memBmp) { SelectObject(g_memDC, g_oldBmp); DeleteObject(g_memBmp); }
    g_memBmp = CreateCompatibleBitmap(g_hdc, g_width, g_height);
    g_oldBmp = (HBITMAP)SelectObject(g_memDC, g_memBmp);
}

inline bool CreateOverlay(HWND target) {
    RECT cr; GetClientRect(target, &cr);
    POINT pt{ 0, 0 }; ClientToScreen(target, &pt);
    g_width  = cr.right - cr.left;
    g_height = cr.bottom - cr.top;

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc   = OverlayProc;
    wc.lpszClassName = L"RbxESPOverlay";
    wc.hInstance     = GetModuleHandleW(nullptr);
    RegisterClassExW(&wc);

    g_overlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        L"RbxESPOverlay", L"", WS_POPUP,
        pt.x, pt.y, g_width, g_height,
        nullptr, nullptr, wc.hInstance, nullptr);

    SetLayeredWindowAttributes(g_overlay, RGB(0,0,0), 0, LWA_COLORKEY);
    ShowWindow(g_overlay, SW_SHOW);
    g_hdc   = GetDC(g_overlay);
    g_memDC = CreateCompatibleDC(g_hdc);
    RebuildMemDC();
    // fuente bold para nombres más visibles
    HFONT fnt = CreateFontA(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
    if (fnt) SelectObject(g_memDC, fnt);
    return g_overlay != nullptr;
}

inline void UpdateOverlayPos(HWND target) {
    RECT cr; GetClientRect(target, &cr);
    POINT pt{ 0, 0 }; ClientToScreen(target, &pt);
    int nw = cr.right - cr.left, nh = cr.bottom - cr.top;
    SetWindowPos(g_overlay, HWND_TOPMOST, pt.x, pt.y, nw, nh, SWP_NOACTIVATE);
    if (nw != g_width || nh != g_height) {
        g_width = nw; g_height = nh;
        RebuildMemDC();
    }
}

inline void ClearOverlay() {
    RECT rc{ 0, 0, g_width, g_height };
    HBRUSH br = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(g_memDC, &rc, br);
    DeleteObject(br);
}

inline void PresentOverlay() {
    BitBlt(g_hdc, 0, 0, g_width, g_height, g_memDC, 0, 0, SRCCOPY);
}

// ---- cache de pens GDI para evitar Create/Delete por cada draw call ----
struct PenCache {
    COLORREF col  = 0xDEADBEEF;
    int      width = 0;
    HPEN     pen  = nullptr;
};
static const int PEN_CACHE_SIZE = 32;
inline PenCache g_penCache[PEN_CACHE_SIZE];
inline int      g_penCacheNext = 0;

inline HPEN GetCachedPen(COLORREF col, int width = 1) {
    for (int i = 0; i < PEN_CACHE_SIZE; i++) {
        if (g_penCache[i].pen && g_penCache[i].col == col && g_penCache[i].width == width)
            return g_penCache[i].pen;
    }
    // evictar el slot más viejo
    int slot = g_penCacheNext % PEN_CACHE_SIZE;
    g_penCacheNext++;
    if (g_penCache[slot].pen) DeleteObject(g_penCache[slot].pen);
    g_penCache[slot] = { col, width, CreatePen(PS_SOLID, width, col) };
    return g_penCache[slot].pen;
}

// ---- primitivas de dibujo (todas sobre g_memDC) ----

inline void DrawLine(Vector2 a, Vector2 b, COLORREF col, int width = 1) {
    HPEN old = (HPEN)SelectObject(g_memDC, GetCachedPen(col, width));
    MoveToEx(g_memDC, (int)a.x, (int)a.y, nullptr);
    LineTo(g_memDC, (int)b.x, (int)b.y);
    SelectObject(g_memDC, old);
}

// Punto sólido — siempre visible incluso a larga distancia
inline void DrawDot(Vector2 center, int r, COLORREF col) {
    HPEN   pen = GetCachedPen(col, 1);
    // brush no se puede cachear fácil — solo para dots que son pequeños, ok crear/borrar
    HBRUSH br  = CreateSolidBrush(col);
    HBRUSH ob  = (HBRUSH)SelectObject(g_memDC, br);
    HPEN   op  = (HPEN)SelectObject(g_memDC, pen);
    ::Ellipse(g_memDC, (int)(center.x-r), (int)(center.y-r),
                       (int)(center.x+r), (int)(center.y+r));
    SelectObject(g_memDC, ob); SelectObject(g_memDC, op);
    DeleteObject(br);
}

inline void DrawEllipse(Vector2 center, float rx, float ry, COLORREF col) {
    HPEN   pen  = GetCachedPen(col, 1);
    HPEN   oldP = (HPEN)SelectObject(g_memDC, pen);
    HBRUSH oldB = (HBRUSH)SelectObject(g_memDC, GetStockObject(NULL_BRUSH));
    ::Ellipse(g_memDC,
        (int)(center.x - rx), (int)(center.y - ry),
        (int)(center.x + rx), (int)(center.y + ry));
    SelectObject(g_memDC, oldP); SelectObject(g_memDC, oldB);
}

inline void DrawBox(Vector2 topLeft, float w, float h, COLORREF col) {
    float lx = topLeft.x, ty = topLeft.y;
    float rx = lx + w,    by = ty + h;
    float sw = w * 0.25f;
    float sh = h * 0.25f;

    HPEN old = (HPEN)SelectObject(g_memDC, GetCachedPen(col, 2));
    MoveToEx(g_memDC,(int)lx,(int)(ty+sh),nullptr); LineTo(g_memDC,(int)lx,(int)ty);
    LineTo(g_memDC,(int)(lx+sw),(int)ty);
    MoveToEx(g_memDC,(int)(rx-sw),(int)ty,nullptr); LineTo(g_memDC,(int)rx,(int)ty);
    LineTo(g_memDC,(int)rx,(int)(ty+sh));
    MoveToEx(g_memDC,(int)rx,(int)(by-sh),nullptr); LineTo(g_memDC,(int)rx,(int)by);
    LineTo(g_memDC,(int)(rx-sw),(int)by);
    MoveToEx(g_memDC,(int)(lx+sw),(int)by,nullptr); LineTo(g_memDC,(int)lx,(int)by);
    LineTo(g_memDC,(int)lx,(int)(by-sh));
    SelectObject(g_memDC, old);
}

// Barra de vida a la izquierda de la box
// top = esquina superior izquierda de la box, h = alto de la box
inline void DrawHealthBar(Vector2 top, float h, float healthPct) {
    float bx  = top.x - 6.f;
    float by  = top.y;
    float bby = top.y + h;
    float filled = by + h * (1.f - healthPct);

    // fondo gris
    HBRUSH bgBr = CreateSolidBrush(RGB(60,60,60));
    RECT bgRc = { (int)bx, (int)by, (int)(bx+4), (int)bby };
    FillRect(g_memDC, &bgRc, bgBr); DeleteObject(bgBr);

    // barra de vida rojo→verde
    int r = (int)(255*(1.f-healthPct));
    int g = (int)(255*healthPct);
    HBRUSH hpBr = CreateSolidBrush(RGB(r,g,0));
    RECT hpRc = { (int)bx, (int)filled, (int)(bx+4), (int)bby };
    FillRect(g_memDC, &hpRc, hpBr); DeleteObject(hpBr);
}

// Texto centrado horizontalmente con outline negro para visibilidad
inline void DrawText2D(Vector2 pos, const std::string& txt, COLORREF col) {
    if (txt.empty()) return;
    SetBkMode(g_memDC, TRANSPARENT);
    int len = (int)txt.size();
    // medir ancho del texto para centrarlo
    SIZE sz{};
    GetTextExtentPoint32A(g_memDC, txt.c_str(), len, &sz);
    int x = (int)pos.x - sz.cx / 2;
    int y = (int)pos.y;
    // outline negro (4 direcciones)
    SetTextColor(g_memDC, RGB(0,0,0));
    for (int dx = -1; dx <= 1; dx++) for (int dy = -1; dy <= 1; dy++)
        if (dx || dy) TextOutA(g_memDC, x+dx, y+dy, txt.c_str(), len);
    // texto principal
    SetTextColor(g_memDC, col);
    TextOutA(g_memDC, x, y, txt.c_str(), len);
}

// Fondo relleno semitransparente para el radar (SRCCOPY no soporta alpha, usamos color oscuro)
inline void DrawFilledCircleBG(Vector2 center, float r) {
    HBRUSH br = CreateSolidBrush(RGB(15, 15, 25));
    HPEN   pn = CreatePen(PS_SOLID, 2, RGB(80, 80, 120));
    HBRUSH ob = (HBRUSH)SelectObject(g_memDC, br);
    HPEN   op = (HPEN)SelectObject(g_memDC, pn);
    ::Ellipse(g_memDC, (int)(center.x-r), (int)(center.y-r),
                       (int)(center.x+r), (int)(center.y+r));
    SelectObject(g_memDC, ob); SelectObject(g_memDC, op);
    DeleteObject(br); DeleteObject(pn);
}

// Cruz de referencia en el radar
inline void DrawRadarCross(Vector2 center, float r) {
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(50, 50, 80));
    HPEN old = (HPEN)SelectObject(g_memDC, pen);
    MoveToEx(g_memDC, (int)(center.x - r), (int)center.y, nullptr);
    LineTo  (g_memDC, (int)(center.x + r), (int)center.y);
    MoveToEx(g_memDC, (int)center.x, (int)(center.y - r), nullptr);
    LineTo  (g_memDC, (int)center.x, (int)(center.y + r));
    SelectObject(g_memDC, old); DeleteObject(pen);
}
