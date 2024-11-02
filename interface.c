#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")  // Link with Winsock library

#define SERVER_IP "172.20.10.10"   // Replace with your Pico W server IP
#define SERVER_PORT 4242           // Port on which picow_tcp_server.c listens

// Global variable to store telemetry data
typedef struct {
    char direction[40];  // Holds combined movement and turn direction
    int speed;           // Holds speed value
} TelemetryData;

TelemetryData telemetry_data = {0};
HWND hwnd;  // Declare the window handle as a global variable

CRITICAL_SECTION telemetryLock;  // Critical section for thread safety

// Function to initialize Winsock and connect to the server
SOCKET connect_to_server() {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct sockaddr_in server_addr;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        MessageBox(NULL, "Winsock initialization failed", "Error", MB_OK | MB_ICONERROR);
        return INVALID_SOCKET;
    }

    // Create a socket
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        MessageBox(NULL, "Socket creation failed", "Error", MB_OK | MB_ICONERROR);
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Set up the server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Connect to the server
    if (connect(ConnectSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        MessageBox(NULL, "Failed to connect to server", "Error", MB_OK | MB_ICONERROR);
        closesocket(ConnectSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);
    return ConnectSocket;
}

// Function to receive telemetry data from the server
DWORD WINAPI receive_telemetry(LPVOID lpParam) {
    SOCKET ConnectSocket = *(SOCKET *)lpParam;
    char recvbuf[2048];
    int bytes_received;

    while (1) {
        // Clear the buffer before each receive to avoid leftover data
        memset(recvbuf, 0, sizeof(recvbuf));

        bytes_received = recv(ConnectSocket, recvbuf, sizeof(recvbuf) - 1, 0);

        if (bytes_received > 0) {
            recvbuf[bytes_received] = '\0';

            // Debug statement to verify data after receiving
            printf("Debug: Received parsed telemetry data: %s\n", recvbuf);

            EnterCriticalSection(&telemetryLock);
            
            // Clear telemetry data before updating
            memset(telemetry_data.direction, 0, sizeof(telemetry_data.direction));
            
            // Parse the direction and speed from the received buffer
            sscanf(recvbuf, "Direction: %39[^;]; Speed: %d", telemetry_data.direction, &telemetry_data.speed);

            LeaveCriticalSection(&telemetryLock);

            // Trigger the window to repaint with the new data
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
        } else if (bytes_received == 0) {
            printf("Connection closed by server\n");
            break;
        } else {
            int error_code = WSAGetLastError();
            printf("Receive error: %d\n", error_code);
            break;
        }
    }

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}

// Window Procedure to handle messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;
    char buffer[256];

    switch (uMsg) {
        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);

            // Clear the background with white color before drawing text
            RECT rect;
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));  // Fills with default window background color

            // Display the direction data
            EnterCriticalSection(&telemetryLock);
            sprintf(buffer, "Direction: %s", telemetry_data.direction);
            TextOut(hdc, 10, 10, buffer, strlen(buffer));

            // Display the speed data on the next line
            sprintf(buffer, "Speed: %d", telemetry_data.speed);
            TextOut(hdc, 10, 30, buffer, strlen(buffer));
            LeaveCriticalSection(&telemetryLock);

            EndPaint(hwnd, &ps);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize the critical section
    InitializeCriticalSection(&telemetryLock);

    // Connect to the server
    SOCKET ConnectSocket = connect_to_server();
    if (ConnectSocket == INVALID_SOCKET) {
        DeleteCriticalSection(&telemetryLock);
        return 1;
    }

    // Initialize window class
    const char CLASS_NAME[] = "TelemetryWindowClass";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create window
    hwnd = CreateWindowEx(0, CLASS_NAME, "Robotic Car Dashboard", WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
                          NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) {
        closesocket(ConnectSocket);
        DeleteCriticalSection(&telemetryLock);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Start telemetry receiving thread
    CreateThread(NULL, 0, receive_telemetry, &ConnectSocket, 0, NULL);

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up the critical section
    DeleteCriticalSection(&telemetryLock);

    return 0;
}
