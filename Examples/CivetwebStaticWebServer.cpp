/* runcpp2

BuildType: InternalExecutable

Dependencies:
-   Name: civetweb
    Platforms: [DefaultPlatform]
    Source:
        Git:
            URL: "https://github.com/civetweb/civetweb.git"
    LibraryType: Static
    IncludePaths:
    -   "./include"
    LinkProperties:
        Windows:
            DefaultProfile:
                SearchLibraryNames: ["civetweb"]
                SearchDirectories: ["./output/src/debug"]
        Unix:
            DefaultProfile:
                SearchLibraryNames: ["civetweb"]
                SearchDirectories: ["./output/src"]
    Setup:
    -   "mkdir output"
    -   "cd output && cmake .. -DCIVETWEB_BUILD_TESTING=OFF -DCIVETWEB_ENABLE_ASAN=OFF"
    -   "cd output && cmake --build . -j 16"
*/

extern "C" {
    #include "civetweb.h"
}

#include <cstring>
#include <stdlib.h>
#include <chrono>
#include <thread>

volatile int exitNow = 0;

int ExitHandler(struct mg_connection *conn, void *cbdata)
{
    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n");
    mg_printf(conn, "Server will shut down.\n");
    mg_printf(conn, "Bye!\n");
    exitNow = 1;
    return 1;
}

int main()
{
    const char *options[] = 
    {
        "document_root",
        ".",
        "listening_ports",
        "8080",
        "request_timeout_ms",
        "10000",
        "error_log_file",
        "error.log",
        "enable_auth_domain_check",
        "no",
        0
    };
    
    mg_callbacks callbacks = {0};
    mg_context* ctx = nullptr;
    mg_server_port ports[32];
    int port_cnt, n;
    int err = 0;
    
    ctx = mg_start(&callbacks, 0, options);
    
    /* Check return value: */
    if (ctx == NULL) 
    {
        fprintf(stderr, "Cannot start CivetWeb - mg_start failed.\n");
        return EXIT_FAILURE;
    }

    mg_set_request_handler(ctx, "/exit", ExitHandler, 0);
    
    /* List all listening ports */
    memset(ports, 0, sizeof(ports));
    port_cnt = mg_get_server_ports(ctx, 32, ports);
    
    printf("\n%i listening ports:\n\n", port_cnt);
    for(n = 0; n < port_cnt && n < 32; n++) 
    {
        const char *proto = ports[n].is_ssl ? "https" : "http";
        const char *host;

        if((ports[n].protocol & 1) == 1) 
        {
            host = "127.0.0.1";
            printf("Browse files at %s://%s:%i/\n", proto, host, ports[n].port);
            printf("Exit at %s://%s:%i%s\n", proto, host, ports[n].port, "/exit");
            printf("\n");
        }
    }
    
    while(!exitNow)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    mg_stop(ctx);
    return 0;
}

