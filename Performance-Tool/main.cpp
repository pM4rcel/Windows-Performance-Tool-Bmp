#include <functional>

#include "system_info.h"
#include "actions.h"
#include "env.h"
#include "processing.h"
#include "sequential.h"
#include "static.h"
#include "dynamic.h"

#include <windows.h>
#include <tchar.h>


TCHAR app_name[] = _T("Windows Performance Test Tool");

int window_height = 700;
int window_width = 1080;

// layout related variables
constexpr int window_padding = 10;
constexpr int btn_height = 48;
constexpr int font_height = 18;
constexpr int label_padding = 4;

std::unique_ptr<TCHAR[]> bmp_file_path = std::unique_ptr<TCHAR[]>(nullptr), st_bmp_cmp = std::unique_ptr<TCHAR
	                         []>(nullptr), nd_bmp_cmp = std::unique_ptr<TCHAR[]>(nullptr);

namespace bitmap{
    BITMAPFILEHEADER file;
    BITMAPINFOHEADER metadata;
}

enum layout{
	logs,
    bmp_stats
};

DWORD cpu_number;

BYTE* mapped_bmp;

RECT rects[bmp_stats + 1];

namespace palette{
    auto gray = RGB(220, 220, 220);
    HBRUSH gray_brush = CreateSolidBrush(gray);

    auto white = RGB(255, 255, 255);
}

enum actions{
    select_file = 1,
    select_st_bmp,
    select_nd_bmp,
    start_cmp,
    select_seq,
    select_static,
    select_dynamic,
    start_test
};

HWND sys_info, test_results, logs_box,img_original, img_gray, img_inverse, start_btn, select_file_btn, comparator_st, comparator_nd, start_comparison;

int selected_test;

int mini_rect_width, mini_rect_height;

HINSTANCE window_instance;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void paint(const HDC&, const RECT&);

int WINAPI _tWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPTSTR lpCmdLine,
    _In_ int nCmdShow
)
{
    env::init();
    WNDCLASSEX wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = palette::gray_brush;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = _T("DesktopApp");
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(nullptr,
            _T("Call to RegisterClassEx failed!"),
            app_name,
            MB_ICONERROR);

        return 1;
    }

    window_instance = hInstance;

    RECT work_area = {};
    SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
    window_width = work_area.right - work_area.left;
    window_height = work_area.bottom - work_area.top;


    const auto hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        _T("DesktopApp"),
        app_name,
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        work_area.left, work_area.top,
        window_width, window_height,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWnd)
    {
        MessageBox(
            nullptr,
            _T("Call to CreateWindow failed!"),
            app_name,
            NULL);

        return EXIT_FAILURE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
    PAINTSTRUCT ps;
    HDC hdc;
    

    RECT client_rect;

    switch (message){


    case WM_CREATE: {

    	GetClientRect(hWnd, &client_rect);

        CreateWindow(
            _T("EDIT"), _T("System Information"), 
            WS_CHILD | WS_VISIBLE | ES_READONLY,
            3 * window_padding, 2 * window_padding + 1 + label_padding,
            ((client_rect.right - 7 * window_padding) >> 1) - 1,
            font_height,
            hWnd, nullptr, nullptr, nullptr
            );

        sys_info = CreateWindow(
            _T("EDIT"), _T("\r\n"),
            WS_CHILD | WS_VISIBLE | ES_READONLY | ES_MULTILINE | WS_VSCROLL,
            3 * window_padding, 2 * window_padding + 2 * label_padding + font_height + 1,
            ((client_rect.right - 7 * window_padding) >> 1) - 1,
            ((client_rect.bottom - 5 * window_padding - 4) >> 1) - font_height - 2 * label_padding,
            hWnd, nullptr, nullptr, nullptr
        );

        CreateWindow(
            _T("EDIT"), _T("Test Results"),
            WS_CHILD | WS_VISIBLE | ES_READONLY,
            3 * window_padding,
            ((client_rect.bottom + window_padding + 2) >> 1) + label_padding,
            ((client_rect.right - 7 * window_padding) >> 1) - 1,
            font_height,
            hWnd, nullptr, nullptr, nullptr
        );

        test_results = CreateWindow(
            _T("EDIT"), nullptr,
            WS_CHILD | WS_VISIBLE | ES_READONLY | ES_MULTILINE | WS_VSCROLL,
            3 * window_padding,
            ((client_rect.bottom + window_padding + 2) >> 1) + font_height + 2 * label_padding + 1,
            ((client_rect.right - 7 * window_padding) >> 1) - 1,
            ((client_rect.bottom - 5 * window_padding - 2) >> 1) - font_height - 2 * label_padding - 1,
            hWnd, nullptr, nullptr, nullptr
        );

        CreateWindow(
            _T("BUTTON"), _T("Select BMP file path"),
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            (client_rect.right + window_padding) >> 1,
            3 * window_padding,
            ((client_rect.right - 5 * window_padding) >> 1),
            btn_height,
            hWnd, reinterpret_cast<HMENU>(select_file),
            nullptr, nullptr
        );

        CreateWindow(
            _T("BUTTON"),
            _T("Sequentially"),
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            (client_rect.right + window_padding) >> 1,
            4 * window_padding + btn_height,
            ((client_rect.right - 5 * window_padding) >> 1) / 3 - 3,
            btn_height,
            hWnd, reinterpret_cast<HMENU>(select_seq),
            nullptr, nullptr
        );

        CreateWindow(
            _T("BUTTON"),
            _T("Static Multi Threaded"),
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            ((client_rect.right + window_padding) >> 1) + ((client_rect.right - 5 * window_padding) >> 1) / 3,
            4 * window_padding + btn_height,
            ((client_rect.right - 5 * window_padding) >> 1) / 3,
            btn_height,
            hWnd, reinterpret_cast<HMENU>(select_static),
            nullptr, nullptr
        );

        CreateWindow(
            _T("BUTTON"),
            _T("Dynamic Multi Threaded"),
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            ((client_rect.right + window_padding) >> 1) + ((client_rect.right - 5 * window_padding)) / 3 + 3,
            4 * window_padding + btn_height,
            ((client_rect.right - 5 * window_padding) >> 1) / 3 - 3,
            btn_height,
            hWnd,
            reinterpret_cast<HMENU>(select_dynamic),
            nullptr, nullptr
        );

        start_btn = CreateWindow(
            _T("BUTTON"), _T("Start"),
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            (client_rect.right + window_padding) >> 1,
            5 * window_padding + 2 * btn_height,
            ((client_rect.right - 5 * window_padding) >> 1),
            btn_height,
            hWnd, reinterpret_cast<HMENU>(start_test),
            nullptr, nullptr
        );

        /*
        LONG left = (client_rect.right + window_padding) >> 1, top = 6 * window_padding + 3 * btn_height;
        rects[layout::logs] = { left, top, left + mini_rect_width, top + mini_rect_height };

        left = rects[layout::logs].right + window_padding;

        rects[layout::bmp_stats] = { left, top, left + mini_rect_width, rects[layout::logs].bottom };
        rects[layout::gray] = { rects[layout::logs].left, rects[layout::logs].bottom + window_padding, rects[layout::logs].right, rects[layout::logs].bottom + window_padding + mini_rect_height };
        rects[layout::inverse] = { rects[layout::bmp_stats].left, rects[layout::gray].top,  rects[layout::bmp_stats].right, rects[layout::gray].bottom };
*/

        comparator_st = CreateWindow(
            _T("BUTTON"), _T("Select First BMP"),
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            (client_rect.right + window_padding) >> 1,
            6 * window_padding + 3 * btn_height,
            ((client_rect.right - 7 * window_padding) >> 2),
            btn_height,
            hWnd, reinterpret_cast<HMENU>(actions::select_st_bmp),
            nullptr, nullptr
        );

        comparator_nd = CreateWindow(
            _T("BUTTON"), _T("Select Second BMP"),
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            ((client_rect.right + window_padding) >> 1) + ((client_rect.right - 3 * window_padding) >> 2),
            6 * window_padding + 3 * btn_height,
            ((client_rect.right - 6 * window_padding) >> 2),
            btn_height,
            hWnd, reinterpret_cast<HMENU>(actions::select_nd_bmp),
            nullptr, nullptr
        );

        start_comparison = CreateWindow(
            _T("BUTTON"), _T("Start Comparison"),
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            (client_rect.right + window_padding) >> 1,
            7 * window_padding + 4 * btn_height,
            ((client_rect.right - 5 * window_padding) >> 1),
            btn_height,
            hWnd, reinterpret_cast<HMENU>(actions::start_cmp),
            nullptr, nullptr
        );

        mini_rect_height = client_rect.bottom - 2 * window_padding;
        mini_rect_width = (((client_rect.right + window_padding) >> 1) - 4 * window_padding) >> 1;

        {
            LONG left = (client_rect.right + window_padding) >> 1;
            constexpr LONG top = 8 * window_padding + 5 * btn_height;
            rects[layout::logs] = { left, top, left + mini_rect_width , client_rect.top + mini_rect_height };

            left = rects[layout::logs].right + window_padding;

            rects[layout::bmp_stats] = { left, top, left + mini_rect_width, rects[layout::logs].bottom };
        }

         CreateWindow(
            _T("EDIT"), _T("Logs"),
            WS_CHILD | WS_VISIBLE | ES_READONLY,
            rects[layout::logs].left + window_padding,
            rects[layout::logs].top + label_padding,
            mini_rect_width - window_padding - 1,
            font_height,
            hWnd,
            nullptr, nullptr, nullptr
        );

         logs_box = CreateWindow(
             _T("EDIT"), _T("\r\n"),
             WS_CHILD | WS_VISIBLE | ES_READONLY | WS_VSCROLL | ES_MULTILINE,
             rects[layout::logs].left + window_padding,
             rects[layout::logs].top + 2 * label_padding + font_height + 1,
             mini_rect_width - window_padding - 1,
             rects[layout::logs].bottom - rects[layout::logs].top - font_height - 2 * label_padding - 2,
             hWnd,
             nullptr, nullptr, nullptr
         );

         CreateWindow(
             _T("EDIT"), _T("Bitmap Stats"),
             WS_CHILD | WS_VISIBLE | ES_READONLY,
             rects[layout::bmp_stats].left + window_padding,
             rects[layout::bmp_stats].top + label_padding,
             mini_rect_width - window_padding - 1,
             font_height,
             hWnd,
             nullptr, nullptr, nullptr
         );

         img_original = CreateWindow(
            _T("EDIT"), _T("\r\n"),
            WS_CHILD | WS_VISIBLE | ES_READONLY | WS_VSCROLL | ES_MULTILINE,
            rects[layout::bmp_stats].left + window_padding,
            rects[layout::bmp_stats].top + 2 * label_padding + font_height + 1,
            mini_rect_width - window_padding - 1,
             rects[layout::bmp_stats].bottom - rects[layout::bmp_stats].top - font_height - 2 * label_padding - 2,
            hWnd,
            nullptr, nullptr, nullptr
        );

        write_data_in(env::system_info_file_path, log_data(sys_info, get_system_sids()), FILE_APPEND_DATA);
        write_data_in(env::system_info_file_path, log_data(sys_info, get_numa_stats()), FILE_APPEND_DATA);
        write_data_in(env::system_info_file_path, log_data(sys_info, get_ht_info()), FILE_APPEND_DATA);
        write_data_in(env::system_info_file_path, log_data(sys_info, get_cpu_sets(cpu_number)), FILE_APPEND_DATA);

        EnableWindow(start_btn, FALSE);
        break;
    }
    case WM_CTLCOLORSTATIC: {
		const auto hdc_static = reinterpret_cast<HDC>(wParam);
        SetBkColor(hdc_static, palette::white);
        return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
    }
    case WM_PAINT: {
            hdc = BeginPaint(hWnd, &ps);

            GetClientRect(hWnd, &client_rect);

            paint(hdc, client_rect);

            EndPaint(hWnd, &ps);
            ReleaseDC(hWnd, hdc);
            break;
        }
    case WM_COMMAND:{
	    switch (const auto action = LOWORD(wParam)) {
	    case select_st_bmp:{
            st_bmp_cmp = get_file(hWnd);
            _sntprintf_s(env::message, env::message_size, _T("Selected First Comparison Path: %s\r\n"), st_bmp_cmp.get());
            log_data(logs_box, env::message);
            break;
	    }
	    case select_nd_bmp: {
            nd_bmp_cmp = get_file(hWnd);
            _sntprintf_s(env::message, env::message_size, _T("Selected Second Comparison Path: %s\r\n"), nd_bmp_cmp.get());
            log_data(logs_box, env::message);
            break;
        }
	    case start_cmp:{
            if (st_bmp_cmp && nd_bmp_cmp) {
                compare_bitmaps(st_bmp_cmp.get(), nd_bmp_cmp.get());
                log_data(logs_box, _T("Finished comparing, generated a file result in D partition!\r\n"));
            }
            else
                log_data(logs_box, _T("You must select two paths, before starting the comparison!\r\n"));
            break;
	    }
        case start_test: {
            const auto get_test = selected_test + select_seq;
            double duration;
            EnableWindow(start_btn, FALSE);
            EnableWindow(select_file_btn, FALSE);
            if (get_test == select_seq) {
                seq_strategy(bmp_file_path.get(), mapped_bmp, bitmap::file, duration);

                _sntprintf_s(env::message, env::message_size, _T("Finished sequential test on \'%s\' in: %f\r\n"), bmp_file_path.get(), duration);

                log_data(test_results, env::message);
            }else if(get_test == select_static){
                static_strategy(bmp_file_path.get(), mapped_bmp, bitmap::file, duration, cpu_number, logs_box);

                _sntprintf_s(env::message, env::message_size, _T("Finished static test on \'%s\' in: %f\r\n"), bmp_file_path.get(), duration);

                log_data(test_results, env::message);
            }else if(get_test == select_dynamic){
                EnableWindow(start_btn, FALSE);
                EnableWindow(select_file_btn, FALSE);
                dynamic_strategy(bmp_file_path.get(), mapped_bmp, bitmap::file, duration, cpu_number, logs_box);

                _sntprintf_s(env::message, env::message_size, _T("Finished dynamic test on \'%s\' in: %f\r\n"), bmp_file_path.get(), duration);

                log_data(test_results, env::message);
            }
            EnableWindow(start_btn, TRUE);
            EnableWindow(select_file_btn, TRUE);
        	break;
        }
    	case select_static:
    	case select_dynamic:
    	case select_seq: {
    		selected_test = action - select_seq;
            _sntprintf_s(env::message, env::message_size, _TRUNCATE, _T("Selected %s test!\r\n"), env::tests[selected_test]);
            log_data(logs_box, env::message);
            break;
    	}
    	case select_file:{
            bmp_file_path = get_file(hWnd);
            selected_test = 0;
            SetWindowText(logs_box, _T("\r\n"));
            SetWindowText(img_original, _T("\r\n"));
            _sntprintf_s(env::message, env::message_size, _T("Selected path: %s\r\n"), bmp_file_path.get());
            log_data(logs_box, env::message);

            if (mapped_bmp){
                UnmapViewOfFile(mapped_bmp);
                mapped_bmp = nullptr;
            }
            if (_tcslen(bmp_file_path.get())) {
                mapped_bmp = load_bmp(bmp_file_path.get(), reinterpret_cast<BYTE*>(&bitmap::file), reinterpret_cast<BYTE*>(&bitmap::metadata));

                EnableWindow(start_btn, _tcslen(bmp_file_path.get()) > 0 ? TRUE : FALSE);

                log_data(img_original, bitmap_stats_stringify(&bitmap::file, &bitmap::metadata));
            }

            break;
    	}
        default:
            break;
    	}
    	break;
    }
    case WM_DESTROY: {
		PostQuitMessage(0);
		break;
    }
    default:
    	return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

void paint(const HDC& hdc, const RECT& window){
    const RECT background = {
                window_padding, window_padding,
                window.right - window_padding, window.bottom - window_padding
    };

    const RECT system_info_frame = { 2 * window_padding, 2 * window_padding, (window.right - window_padding) >> 1, (window.bottom - window_padding) >> 1 };

    const RECT test_results_frame = { system_info_frame.left, (window.bottom + window_padding) >> 1, system_info_frame.right, window.bottom - 2 * window_padding };

    FillRect(hdc, &background, nullptr);
    FrameRect(hdc, &system_info_frame, palette::gray_brush);
    FrameRect(hdc, &test_results_frame, palette::gray_brush);

    const auto pen_handle = CreatePen(PS_SOLID, 1, palette::gray);
    for (const auto& rect : rects) {
        FrameRect(hdc, &rect, palette::gray_brush);

        SelectObject(hdc, pen_handle);
        MoveToEx(hdc, rect.left, rect.top + font_height + 2 * label_padding, nullptr);
        LineTo(hdc, rect.right, rect.top + font_height + 2 * label_padding);
    }
	

    SelectObject(hdc, pen_handle);

    MoveToEx(hdc, system_info_frame.left, system_info_frame.top + font_height + 2 * label_padding, nullptr);
    LineTo(hdc, system_info_frame.right, system_info_frame.top + font_height + 2 * label_padding);

	MoveToEx(hdc, test_results_frame.left, test_results_frame.top + 2 * label_padding + font_height, nullptr);
    LineTo(hdc, test_results_frame.right, test_results_frame.top + 2 * label_padding + font_height);

    DeleteObject(pen_handle);
}




