#pragma once

#include <d2d1_3.h>
#include <dwrite.h>
#include "util.h"

#define FONT_NAME L"Consolas"

template<class Interface> inline void SafeRelease(Interface** ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = NULL;
	}
}

class graphic {
public:
	ID2D1Factory* m_pD2DFactory;
	IWICImagingFactory* m_pWICFactory;
	IDWriteFactory* m_pDWriteFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;
	IDWriteTextFormat* m_pTextFormat;
	ID2D1SolidColorBrush* m_pNormalBrush;
	ID2D1SolidColorBrush* m_pSelectBrush;
	ID2D1SolidColorBrush* m_pNormalBkBrush;
	ID2D1SolidColorBrush* m_pSelectBkBrush;
	ID2D1SolidColorBrush* m_pGridBrush;
	ID2D1SolidColorBrush* m_pConnectPointBrush;
	ID2D1SolidColorBrush* m_pConnectPointBkBrush;
	graphic(HWND hWnd)
		: m_pD2DFactory(0)
		, m_pWICFactory(0)
		, m_pDWriteFactory(0)
		, m_pRenderTarget(0)
		, m_pTextFormat(0)
		, m_pNormalBrush(0)
		, m_pSelectBrush(0)
		, m_pNormalBkBrush(0)
		, m_pSelectBkBrush(0)
		, m_pGridBrush(0)
		, m_pConnectPointBrush(0)
		, m_pConnectPointBkBrush(0)
	{
		static const FLOAT msc_fontSize = 25;

		HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
		if (SUCCEEDED(hr))
			hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
		if (SUCCEEDED(hr))
			hr = m_pDWriteFactory->CreateTextFormat(FONT_NAME, 0, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, msc_fontSize, L"", &m_pTextFormat);
		if (SUCCEEDED(hr))
			hr = m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		if (SUCCEEDED(hr))
			hr = m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		if (FAILED(hr)) {
			MessageBox(hWnd, L"Direct2D ‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½B", 0, 0);
		}
	}
	~graphic() {
		SafeRelease(&m_pD2DFactory);
		SafeRelease(&m_pDWriteFactory);
		SafeRelease(&m_pRenderTarget);
		SafeRelease(&m_pTextFormat);
		SafeRelease(&m_pNormalBrush);
		SafeRelease(&m_pSelectBrush);
		SafeRelease(&m_pNormalBkBrush);
		SafeRelease(&m_pSelectBkBrush);
		SafeRelease(&m_pGridBrush);
		SafeRelease(&m_pConnectPointBrush);
		SafeRelease(&m_pConnectPointBkBrush);
	}
	bool begindraw(HWND hWnd) {
		HRESULT hr = S_OK;
		if (!m_pRenderTarget)
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			D2D1_SIZE_U size = D2D1::SizeU(rect.right, rect.bottom);
			hr = m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(), DEFAULT_DPI, DEFAULT_DPI, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT), D2D1::HwndRenderTargetProperties(hWnd, size), &m_pRenderTarget);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pNormalBrush);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &m_pSelectBrush);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.75f), &m_pNormalBkBrush);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.25f), &m_pSelectBkBrush);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.80f, 0.80f, 0.80f, 1.00f), &m_pGridBrush);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.00f), &m_pConnectPointBrush);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.75f), &m_pConnectPointBkBrush);
		}
		if (SUCCEEDED(hr))
		{
			m_pRenderTarget->BeginDraw();
		}
		return SUCCEEDED(hr);
	}
	void enddraw() {
		HRESULT hr = m_pRenderTarget->EndDraw();
		if (hr == D2DERR_RECREATE_TARGET)
		{
			SafeRelease(&m_pRenderTarget);
			SafeRelease(&m_pNormalBrush);
			SafeRelease(&m_pSelectBrush);
			SafeRelease(&m_pNormalBkBrush);
			SafeRelease(&m_pSelectBkBrush);
			SafeRelease(&m_pGridBrush);
			SafeRelease(&m_pConnectPointBrush);
			SafeRelease(&m_pConnectPointBkBrush);
		}
	}
};
