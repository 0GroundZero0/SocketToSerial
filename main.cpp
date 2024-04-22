#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
using namespace std;

const int    DEFAULT_BUFLEN =       512;
const char*  DEFAULT_NET_PORT =     "8000"; 

char lastErr[1020];

int main(int argc, char** argv) {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    int iResult;



    // Validate the parameters
    if (argc != 3) {
        printf("usage: %s server-name serial-name\n", argv[0]);
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    
    iResult = getaddrinfo(argv[1], DEFAULT_NET_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }
    //Open serial port
    
    HANDLE h_Serial;
    h_Serial = CreateFileA(argv[2], GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, 0);
    if (h_Serial == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            // serial port not found. Handle error here.
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                lastErr, 1020, NULL);
            printf("%s", lastErr);
            return 1;
        }
        // any other error. Handle error here.
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
            GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            lastErr, 1020, NULL);
        printf("%s", lastErr);
        cerr << "Unknown Exception" << endl;
        return 1;
    }

    DCB dcbSerialParam = { 0 };
    dcbSerialParam.DCBlength = sizeof(dcbSerialParam);

    if (!GetCommState(h_Serial, &dcbSerialParam)) {
        // handle error here
    }

    dcbSerialParam.BaudRate = CBR_9600;
    dcbSerialParam.ByteSize = 8;
    dcbSerialParam.StopBits = ONESTOPBIT;
    dcbSerialParam.Parity = NOPARITY;

    if (!SetCommState(h_Serial, &dcbSerialParam)) {
        // handle error here
    }

    COMMTIMEOUTS timeout = { 0 };
    timeout.ReadIntervalTimeout = 60;
    timeout.ReadTotalTimeoutConstant = 60;
    timeout.ReadTotalTimeoutMultiplier = 15;
    timeout.WriteTotalTimeoutConstant = 60;
    timeout.WriteTotalTimeoutMultiplier = 8;
    if (!SetCommTimeouts(h_Serial, &timeout)) {
        // handle error here
    }




    char* writeBuff = new char[DEFAULT_BUFLEN];
    char* readBuff = new char[DEFAULT_BUFLEN];
    DWORD numWritten = 0;
    DWORD numRead = 0;
    // Receive until the peer closes the connection
    do {
        char* searchBuff;
        do {
            iResult = recv(ConnectSocket, writeBuff, DEFAULT_BUFLEN, MSG_PEEK);
            searchBuff = strstr(writeBuff, "wheel = ");
        } while (searchBuff == NULL);
        
        iResult = recv(ConnectSocket, writeBuff, DEFAULT_BUFLEN, 0);
        searchBuff = strstr(writeBuff, "wheel = ");
        char tmpBuff[3];
        
        if (searchBuff != NULL) {
            //motor = *(searchBuff + 8);
            for (int i = 8; i < 11; i++) {
                tmpBuff[i - 8] = searchBuff[i];
            }
            uint8_t motor = atoi(tmpBuff);
            printf("%i\n", motor);
            if (!WriteFile(h_Serial, searchBuff, 11, &numWritten, NULL)) {
                FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                    GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    lastErr, 1020, NULL);
                printf("%s", lastErr);
            }
            if (!ReadFile(h_Serial, readBuff, numWritten, &numRead, NULL)) {
                FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                    GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    lastErr, 1020, NULL);
                printf("%s", lastErr);
            }
        }
        //if (numRead != '0') {
            
        //}

        printf("\n");
        if (iResult > 0) {
            //printf("Bytes received: %d\n", iResult);
        }
        else if (iResult == 0) {
            printf("Connection closed\n");
        }
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
        }

    } while (iResult > 0);

    
    
    
    // cleanup
    CloseHandle(h_Serial);
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
