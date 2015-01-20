int wsa_init() {
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2,0),&wsa);
}

int tcp_bind(SOCKET *sock, const char * host, uint16_t port) {
    if (wsa_init())
        return 1;
    SOCKADDR_IN addr;
    hostent* he = gethostbyname(host);
    if (!he) {
		cerr << "[!] gethostbyname(\"" << host << "\") failed" << endl;
        return 1;
	}
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    *sock = WSASocketA(AF_INET, SOCK_STREAM, 0, 0, 0, 0);
    if (*sock == INVALID_SOCKET) {
		cerr << "[!] WSASocketA failed" << endl;
        return 1;
	}
	int reuse = 1;
	if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof reuse) < 0) {
		cerr << "[!] setsockopt(SO_REUSEADDR) failed" << endl;
		return 1;
	}
    if (bind(*sock, (SOCKADDR*)&addr, sizeof(addr))) {
		cerr << "[!] bind failed" << endl;
		return 1;
	}
    if (listen(*sock, 1)) {
		cerr << "[!] listen failed" << endl;
		return 1;
	}
    return 0;
}

int tcp_accept(SOCKET server, SOCKET *sock) {
	*sock = accept(server, 0, 0);
    if (*sock == INVALID_SOCKET)
        return 1;
    DWORD non_blocking_l = 1;
    if (ioctlsocket(*sock, FIONBIO, &non_blocking_l))
        return 1;
	return 0;
}

int recv_blocking(SOCKET sock, char* buf, size_t len, int flags) {
    for (;;) {
        FD_SET read_set;
        FD_ZERO(&read_set);
        FD_SET(sock, &read_set);
        TIMEVAL timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500;
        if (select(0, &read_set, NULL, NULL, &timeout))
            break;
    }
    return recv(sock, buf, len, flags);
}

int tcp_connect(SOCKET *sock, const char * host, uint16_t port) {
    if (wsa_init())
        return 1;
    SOCKADDR_IN addr;
    hostent* he = gethostbyname(host);
    if (!he) 
        return 1;
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    *sock = WSASocketA(AF_INET, SOCK_STREAM, 0, 0, 0, 0);
    if (*sock == INVALID_SOCKET)
        return 1;
    if (connect(*sock, (SOCKADDR*)&addr, sizeof(addr)))
        return 1;
    DWORD non_blocking_l = 1;
    if (ioctlsocket(*sock, FIONBIO, &non_blocking_l))
        return 1;
    return 0;
}