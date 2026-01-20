#include <windows.h>
#include <time.h>
#include <math.h>
#include <vector>

// --- Variáveis e Estruturas Globais ---
static ULONGLONG n, r;
int randy() {
    return n = r, n ^= 0x8ebf635bee3c6d25, n ^= n << 5 | n >> 26, n *= 0xf3e05ca5c43e376b, r = n, n & 0x7fffffff;
}

struct HSL {
    float h, s, l;
};

// --- Funções de Conversão de Cor ---
HSL RGBtoHSL(COLORREF rgb) {
    float r = GetRValue(rgb) / 255.0f;
    float g = GetGValue(rgb) / 255.0f;
    float b = GetBValue(rgb) / 255.0f;
    float max = fmaxf(r, fmaxf(g, b)), min = fminf(r, fminf(g, b));
    float h, s, l = (max + min) / 2.0f;
    if (max == min) h = s = 0;
    else {
        float d = max - min;
        s = l > 0.5f ? d / (2.0f - max - min) : d / (max + min);
        if (max == r) h = (g - b) / d + (g < b ? 6 : 0);
        else if (max == g) h = (r - b) / d + 2;
        else h = (r - g) / d + 4;
        h /= 6.0f;
    }
    return { h, s, l };
}

float HueToRGB(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
}

COLORREF HSLtoRGB(HSL hsl) {
    float r, g, b;
    if (hsl.s == 0) r = g = b = hsl.l;
    else {
        float q = hsl.l < 0.5f ? hsl.l * (1.0f + hsl.s) : hsl.l + hsl.s - hsl.l * hsl.s;
        float p = 2.0f * hsl.l - q;
        r = HueToRGB(p, q, hsl.h + 1.0f / 3.0f);
        g = HueToRGB(p, q, hsl.h);
        b = HueToRGB(p, q, hsl.h - 1.0f / 3.0f);
    }
    return RGB((int)(r * 255), (int)(g * 255), (int)(b * 255));
}

// --- Threads de Payloads ---

// 1. Deslocamento Vertical (Imediato)
DWORD WINAPI payloadVertical(LPVOID lpParam) {
    while (1) {
        HDC hdc = GetDC(NULL);
        int w = GetSystemMetrics(0), h = GetSystemMetrics(1);
        int rx = rand() % w;
        BitBlt(hdc, rx, 10, 100, h, hdc, rx, 0, SRCCOPY);
        ReleaseDC(NULL, hdc);
        Sleep(5);
    }
}

// 2. Túnel Horizontal (Imediato)
DWORD WINAPI payloadHorizontal(LPVOID lpParam) {
    while (1) {
        int w = GetSystemMetrics(0), h = GetSystemMetrics(1);
        HDC hdc = GetDC(0);
        BitBlt(hdc, -30, 0, w, h, hdc, 0, 0, SRCCOPY);
        BitBlt(hdc, w - 30, 0, w, h, hdc, 0, 0, SRCCOPY);
        ReleaseDC(0, hdc);
        Sleep(10);
    }
}

// 3. GDI Mistos (Espera 8s)
DWORD WINAPI payloadGDI(LPVOID lpParam) {
    Sleep(8000);
    const char* phrases[] = { "BURN", "GET OUT.", "FIRE IN THE HOLE", "MOTHERFUCKER", "PEE NOW" };
    while (1) {
        int sw = GetSystemMetrics(0), sh = GetSystemMetrics(1);
        HDC hdc = GetDC(0);
        SetTextColor(hdc, RGB(rand() % 255, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        TextOutA(hdc, rand() % sw, rand() % sh, phrases[rand() % 5], lstrlenA(phrases[rand() % 5]));
        BitBlt(hdc, rand() % 10 - 5, rand() % 10 - 5, sw, sh, hdc, 0, 0, SRCCOPY);
        HBRUSH brush = CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255));
        SelectObject(hdc, brush);
        Pie(hdc, rand() % sw, rand() % sh, rand() % sw, rand() % sh, rand() % sw, rand() % sh, rand() % sw, rand() % sh);
        DeleteObject(brush);
        ReleaseDC(0, hdc);
        Sleep(30);
    }
}

// 4. Distorção RGB (Espera 20s)
DWORD WINAPI payloadRGB(LPVOID lpParam) {
    Sleep(20000);
    int w = GetSystemMetrics(0), h = GetSystemMetrics(1);
    RGBQUAD* data = (RGBQUAD*)VirtualAlloc(0, (w * h + w) * sizeof(RGBQUAD), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    for (int i = 0;; i++, i %= 3) {
        HDC desk = GetDC(NULL), hdcdc = CreateCompatibleDC(desk);
        HBITMAP hbm = CreateBitmap(w, h, 1, 32, data);
        SelectObject(hdcdc, hbm);
        BitBlt(hdcdc, 0, 0, w, h, desk, 0, 0, SRCCOPY);
        GetBitmapBits(hbm, w * h * 4, data);
        int v = 0;
        for (int j = 0; w * h > j; j++) {
            if (j % h == 0 && randy() % 100 == 0) v = randy() % 50;
            ((BYTE*)(data + j))[v % 3] += ((BYTE*)(data + j + v))[v];
        }
        SetBitmapBits(hbm, w * h * 4, data);
        BitBlt(desk, 0, 0, w, h, hdcdc, 0, 0, SRCCOPY);
        DeleteObject(hbm); DeleteDC(hdcdc); ReleaseDC(NULL, desk);
        Sleep(10);
    }
}

// 5. Rotação de Hue HSL (Espera 40s para efeito final)
DWORD WINAPI payloadHSL(LPVOID lpParam) {
    Sleep(40000); // Começa após a distorção RGB estar bem avançada
    int w = GetSystemMetrics(0), h = GetSystemMetrics(1);
    float hueShift = 0.01f;
    while (true) {
        HDC hdc = GetDC(NULL), hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbm = CreateCompatibleBitmap(hdc, w, h);
        SelectObject(hdcMem, hbm);
        BitBlt(hdcMem, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w; bmi.bmiHeader.biHeight = -h;
        bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biBitCount = 32;
        std::vector<RGBQUAD> pixels(w * h);
        GetDIBits(hdcMem, hbm, 0, h, &pixels[0], &bmi, DIB_RGB_COLORS);
        for (int i = 0; i < w * h; i++) {
            HSL hsl = RGBtoHSL(RGB(pixels[i].rgbRed, pixels[i].rgbGreen, pixels[i].rgbBlue));
            hsl.h = fmodf(hsl.h + hueShift, 1.0f);
            COLORREF newRgb = HSLtoRGB(hsl);
            pixels[i].rgbRed = GetRValue(newRgb); pixels[i].rgbGreen = GetGValue(newRgb); pixels[i].rgbBlue = GetBValue(newRgb);
        }
        SetDIBits(hdcMem, hbm, 0, h, &pixels[0], &bmi, DIB_RGB_COLORS);
        BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);
        DeleteObject(hbm); DeleteDC(hdcMem); ReleaseDC(NULL, hdc);
        Sleep(20);
    }
}

int main() {
    srand(time(NULL));
    // ShowWindow(GetConsoleWindow(), SW_HIDE);
    CreateThread(NULL, 0, payloadVertical, NULL, 0, NULL);
    CreateThread(NULL, 0, payloadHorizontal, NULL, 0, NULL);
    CreateThread(NULL, 0, payloadGDI, NULL, 0, NULL);
    CreateThread(NULL, 0, payloadRGB, NULL, 0, NULL);
    CreateThread(NULL, 0, payloadHSL, NULL, 0, NULL);
    while (1) Sleep(1000);
    return 0;
}