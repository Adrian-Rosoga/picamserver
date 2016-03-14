#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <functional>

#include "ImageMgr.h"
#include "producer_consumer.h"
#include "util.h"

#define ASSERTNOERR(x, msg) do { if ((x) == -1) { std::cout << "\nAsser failed!\n"; perror(msg); exit(-1); } } while (0)

#define TEST_WRITE_SOCKET(n) if ((n) < 0)\
{ std::cerr << "Error: Writing to socket: Line " << __LINE__ << std::endl; }

//const std::string RaspistillCmd = "raspistill -n -q 100 -w 1440 -h 1080 -o -";
const std::string RaspistillCmd = "raspistill -n -q 100 -w 720 -h 540 -o -";
const int LISTEN_BACKLOG = 20;

const std::string HTTP_RESPONSE =
R"foo(HTTP/1.0 200 OK
Server: picamserver
Content-type: text/html

<!DOCTYPE html>
<HTML>
    <HEAD>
        <TITLE>PiCamServer</TITLE>
        <META HTTP-EQUIV=PRAGMA CONTENT=NO-CACHE>
        <META HTTP-EQUIV=EXPIRES CONTENT=-1>
        <META HTTP-EQUIV=REFRESH CONTENT=10>
    </HEAD>
    <BODY BGCOLOR="BLACK">
        <H2 ALIGN="CENTER">
        <B><FONT COLOR="YELLOW">PiCamServer</FONT></B>
        </H2>
        <P ALIGN="CENTER">
        <IMG SRC="picamimage" STYLE="HEIGHT: 100%" ALT="NO IMAGE!" WIDTH="720" HEIGHT="540">
        </P>
    </BODY>
</HTML>
)foo";

const std::string HTTP_HEADER_200_OK =
R"foo(HTTP/1.1 200 OK
Server: picamserver
Accept-Ranges: bytes
)foo";

const std::string HTTP_HEADER_404_NOT_FOUND =
R"foo(HTTP/1.1 404 Not Found
Server: picamserver)foo";

const std::string HTTP_BODY_404_NOT_FOUND =
R"foo(<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">
<html><head>
<title>404 Not Found</title>
</head><body>
<h1>Not Found</h1>
<p>The requested URL was not found.</p>
</body></html>
)foo";


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        exit(1);
    }
    
    const int port = atoi(argv[1]);
    
    auto s = socket(AF_INET, SOCK_STREAM, 0);
    
    ASSERTNOERR(s, "socket");
    
    int reuse = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    #ifdef SO_REUSEPORT
    std::cout << "SO_REUSEPORT is defined" << std::endl;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) 
        perror("setsockopt(SO_REUSEPORT) failed");
    #endif
    
    struct sockaddr_in my_addr;
    
    memset(&my_addr, 0, sizeof(struct sockaddr));

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    
    int rc = bind(s, (struct sockaddr*)&my_addr, sizeof(my_addr));
    
    ASSERTNOERR(rc, "bind");
    
    rc = listen(s, LISTEN_BACKLOG);
    
    ASSERTNOERR(rc, "listen");
    
    ImageMgr imageMgr;
    imageMgr.startImageThread(RaspistillCmd);
      
    while (1)
    {
        fd_set fds;
        
        FD_ZERO(&fds);        
        FD_SET(s, &fds);
        
        int rc = select(s + 1, &fds, nullptr, nullptr, nullptr);
        
        ASSERTNOERR(rc, "select");
        
        struct sockaddr_in peer_adr;
        socklen_t addrlen = sizeof(peer_adr);
        
        int fd = accept(s, (struct sockaddr*)&peer_adr, &addrlen);
        
        ASSERTNOERR(fd, "accept");
        
        std::string request;
        request.reserve(1024);
        
        char buf[256];
        while (1)
        {
            auto n = read(fd, buf, sizeof(buf) - 1);
            if (n == 0) { break; }
            
            bool read_all = false;
            
            // Find the end of the headers, i.e. \r\n\r\n - make it work with '\n\n' as well
            if (n >=4 && buf[n-4] == '\r' && buf[n-3] == '\n' && buf[n-2] == '\r' && buf[n-1] == '\n')
            {
                read_all = true;
            }
            
            buf[n] = '\0';
            
            request += buf;
            
            if (read_all) { break; }
        }
        
        // Print the request line
        auto pos = request.find_first_of('\n');
        if (pos != std::string::npos)
        {
            //std::cout << request.substr(0, pos) << std::endl;
        }
        
        int n = 0;
        if (request.find("GET / HTTP") != std::string::npos)
        {
            std::string page(HTTP_RESPONSE);
            
            n = write(fd, page.c_str(), page.size());
            TEST_WRITE_SOCKET(n);
        }
        else if (request.find("GET /picamimage HTTP") != std::string::npos)
        {
            n = write(fd, HTTP_HEADER_200_OK.c_str(), HTTP_HEADER_200_OK.size());
            TEST_WRITE_SOCKET(n);
            
            std::ostringstream str;
            
            str << "Content-Type: image/jpeg" << "\n";
            
            std::function<void(int)> callback = [&](int index) {
                        
                str << "Content-lenght: " << imageMgr[index].size << "\n\n";
                n = write(fd, str.str().c_str(), str.str().size());
                TEST_WRITE_SOCKET(n);
                
                n = write(fd, imageMgr[index].getBuffer(), imageMgr[index].size);
                TEST_WRITE_SOCKET(n);
                
                log(std::this_thread::get_id(), " consume: index=", index);
            };
            
            consume(callback);
            
        }
        else if (request.find("GET /favicon.ico HTTP") != std::string::npos)
        {
            n = write(fd, HTTP_HEADER_200_OK.c_str(), HTTP_HEADER_200_OK.size());
            TEST_WRITE_SOCKET(n);
            
            std::ostringstream str;
            
            std::ifstream fav("favicon.ico", std::ios::binary);           
            if (!fav)
            {
                std::cerr << "Cannot open favicon.ico" << std::endl;
                str << "Content-lenght: " << 0 << "\n\n";
                continue;
            }
            
            fav.seekg(0, std::ios::end);
            const int size = fav.tellg();
                 
            str << "Content-Type: image/ico" << "\n"; 
            str << "Content-lenght: " << size << "\n\n";           
            n = write(fd, str.str().c_str(), str.str().size());
            TEST_WRITE_SOCKET(n);
            
            fav.seekg(0, std::ios::beg);
            
            const int BUFFER_SIZE = 1024;
            char buffer[BUFFER_SIZE];
            while (fav.read(buffer, BUFFER_SIZE))
            {
                n = write(fd, buffer, fav.gcount());
                TEST_WRITE_SOCKET(n);
            }
        }
        else
        {
            // Bail out  
            std::ostringstream str;
            str << HTTP_HEADER_404_NOT_FOUND << "\n";
            str << "Content-lenght: " << HTTP_BODY_404_NOT_FOUND.size() << "\n\n";
            str << HTTP_BODY_404_NOT_FOUND;
            
            std::cout << str.str() << std::endl;
            
            n = write(fd, str.str().c_str(), str.str().size());
            TEST_WRITE_SOCKET(n);
            
            std::cout << "Error: Unknown request: '" << request << "'" << std::endl;
        }
        
        close(fd);
    }
    
    close(s);
    
    return 0;
}
