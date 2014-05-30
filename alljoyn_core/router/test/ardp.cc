/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ******************************************************************************/
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <vector>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Event.h>
#include <qcc/Socket.h>
#include <qcc/SocketTypes.h>
#include <qcc/Thread.h>

#include <alljoyn/Status.h>

#include <ArdpProtocol.h>

#define QCC_MODULE "ARDP"

using namespace ajn;

const uint32_t UDP_CONNECT_TIMEOUT = 3000;  /**< How long before we expect a connection to complete */
const uint32_t UDP_CONNECT_RETRIES = 3;     /**< How many times do we retry a connection before giving up */
const uint32_t UDP_DATA_TIMEOUT = 3000;     /**< How long do we wait before retrying sending data */
const uint32_t UDP_DATA_RETRIES = 5;        /**< How many times to we try do send data before giving up and terminating a connection */
const uint32_t UDP_PERSIST_TIMEOUT = 5000;  /**< How long do we wait before pinging the other side due to a zero window */
const uint32_t UDP_PERSIST_RETRIES = 5;     /**< How many times do we do a zero window ping before giving up and terminating a connection */
const uint32_t UDP_PROBE_TIMEOUT = 3000;    /**< How long to we wait on an idle link before generating link activity */
const uint32_t UDP_PROBE_RETRIES = 5;       /**< How many times do we try to probe on an idle link before terminating the connection */
const uint32_t UDP_DUPACK_COUNTER = 1;      /**< How many duplicate acknowledgements to we need to trigger a data retransmission */
const uint32_t UDP_TIMEWAIT = 1000;         /**< How long do we stay in TIMWAIT state before releasing the per-connection resources */

bool g_user = false;
char const* g_localport = "9954";
char const* g_foreignport = "9955";
char const* g_address = "127.0.0.1";

char const* g_ajnConnString = "AUTH ANONIMOUS; BEGIN; Bus Hello";
char const* g_ajnAcceptString = "OK 123455678; Hello";

static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

bool AcceptCb(ArdpHandle* handle, qcc::IPAddress ipAddr, uint16_t ipPort, ArdpConnRecord* conn, uint8_t* buf, uint16_t len, QStatus status)
{
    QCC_DbgTrace(("AcceptCb(handle=%p, ipAddr=\"%s\", foreign=%d, conn=%p, buf=%p(\"%s\"), len=%d, status=%s)",
                  handle, ipAddr.ToString().c_str(), ipPort, conn, buf, (char*) buf, len, QCC_StatusText(status)));

    uint16_t length = random() % ARDP_USRBMAX;
    uint8_t* buffer = new uint8_t[length];
    status = ARDP_Accept(handle, conn, ARDP_SEGMAX, ARDP_SEGBMAX, buffer, length);
    if (status != ER_OK) {
        QCC_DbgPrintf(("AcceptCb(): ARDP_Accept failed with %s", QCC_StatusText(status)));
    }
    return true;
}

void ConnectCb(ArdpHandle* handle, ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status)
{
    QCC_DbgTrace(("ConnectCb(handle=%p, conn=%p, passive=%s, buf=%p, len=%d, status=%s)",
                  handle, conn, (passive) ? "true" : "false", buf, len, QCC_StatusText(status)));
    if (status == ER_OK) {
        if (!passive) {
            QCC_DbgPrintf(("ConnectCb: response string \"%s\"", (char*)buf));
        }

        uint16_t length = random() % ARDP_USRBMAX;
        uint8_t* buffer = new uint8_t[length];
        QCC_DbgPrintf(("ConnectCb(): ARDP_Send(handle=%p, conn=%p, buffer=%p, length=%d)", handle, conn, buffer, length));

        status = ARDP_Send(handle, conn, buffer, length, ARDP_TTL_PLACEHOLDER);
        if (status != ER_OK) {
            QCC_DbgPrintf(("ConnectCb(): ARDP_Send failed with %s", QCC_StatusText(status)));
        }
    } else {
        QCC_DbgPrintf(("Connect establishment failed"));
    }
}

void DisconnectCb(ArdpHandle* handle, ArdpConnRecord* conn, QStatus status)
{
    QCC_DbgTrace(("DisconnectCb(handle=%p, conn=%p, status=%s)", handle, conn, QCC_StatusText(status)));
}

void RecvCb(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* rcv, QStatus status)
{
    ArdpRcvBuf* buf = rcv;
    uint32_t len = 0;
    uint16_t cnt = rcv->fcnt;
    QCC_DbgTrace(("RecvCb(handle=%p, conn=%p, rcv=%p, status=%s)",
                  handle, conn, rcv, QCC_StatusText(status)));
    /* Consume data buffers */
    for (uint16_t i; i < cnt; i++) {
        QCC_DbgPrintf(("RecvCb(): got %d bytes of data", buf->datalen));
        len += buf->datalen;
        buf = buf->next;
    }
    QCC_DbgPrintf(("RecvCb(): got TOTAL %d bytes of data", len));
    ARDP_RecvReady(handle, conn, rcv);
}

void SendCb(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status)
{
    QCC_DbgTrace(("SendCb(handle=%p, conn=%p, buf=%p, len=%d, status=%s)",
                  handle, conn, buf, len, QCC_StatusText(status)));
    delete buf;
    len = 0;
    uint16_t length = random() % ARDP_USRBMAX;
    uint8_t* buffer = new uint8_t[length];
    QCC_DbgTrace(("SendCb(): ARDP_Send(handle=%p, conn=%p, buffer=%p, length=%d.)", handle, conn, buffer, length));

    status = ARDP_Send(handle, conn, buffer, length, ARDP_TTL_PLACEHOLDER);
    if (status != ER_OK) {
        QCC_DbgPrintf(("SendCb(): ARDP_Send failed with %s", QCC_StatusText(status)));
    }
}

void SendWindowCb(ArdpHandle* handle, ArdpConnRecord* conn, uint16_t window, QStatus status)
{
    QCC_DbgTrace(("SendWindowCb(handle=%p, conn=%p, window=%d, status=%s)",
                  handle, conn, window, QCC_StatusText(status)));
}

class Test : public qcc::Thread {
  public:
    QStatus Start();
    qcc::ThreadReturn STDCALL Run(void* arg);
};

QStatus Test::Start()
{
    QCC_DbgTrace(("Test::Start()"));
    return Thread::Start(this);
}

void* Test::Run(void* arg)
{
    qcc::SocketFd sock;

    QStatus status = qcc::Socket(qcc::QCC_AF_INET, qcc::QCC_SOCK_DGRAM, sock);
    if (status != ER_OK) {
        QCC_LogError(status, ("Test::Run(): Socket(): Failed"));
        return 0;
    }

    status = qcc::SetBlocking(sock, false);
    if (status != ER_OK) {
        QCC_LogError(status, ("Test::Run(): SetBlocking(): Failed"));
        return 0;
    }

    status = qcc::Bind(sock, qcc::IPAddress("0.0.0.0"), atoi(g_localport));
    if (status != ER_OK) {
        QCC_LogError(status, ("Test::Run(): Bind(): Failed"));
        return 0;
    }

    ArdpGlobalConfig config;
    config.connectTimeout = UDP_CONNECT_TIMEOUT;
    config.connectRetries = UDP_CONNECT_RETRIES;
    config.dataTimeout = UDP_DATA_TIMEOUT;
    config.dataRetries = UDP_DATA_RETRIES;
    config.persistTimeout = UDP_PERSIST_TIMEOUT;
    config.persistRetries = UDP_PERSIST_RETRIES;
    config.probeTimeout = UDP_PROBE_TIMEOUT;
    config.probeRetries = UDP_PROBE_RETRIES;
    config.dupackCounter = UDP_DUPACK_COUNTER;
    config.timewait = UDP_TIMEWAIT;

    ArdpHandle* handle = ARDP_AllocHandle(&config);
    ARDP_SetAcceptCb(handle, AcceptCb);
    ARDP_SetConnectCb(handle, ConnectCb);
    ARDP_SetDisconnectCb(handle, DisconnectCb);
    ARDP_SetRecvCb(handle, RecvCb);
    ARDP_SetSendCb(handle, SendCb);
    ARDP_SetSendWindowCb(handle, SendWindowCb);

    ARDP_StartPassive(handle);

    qcc::Event* sockEvent = new qcc::Event(sock, qcc::Event::IO_READ, false);
    qcc::Event timerEvent(1000, 1000);

    bool connectSent = false;

    while (IsRunning()) {
        std::vector<qcc::Event*> checkEvents, signaledEvents;
        checkEvents.push_back(&stopEvent);
        checkEvents.push_back(&timerEvent);
        checkEvents.push_back(sockEvent);

        QStatus status = qcc::Event::Wait(checkEvents, signaledEvents);
        if (status != ER_OK && status != ER_TIMEOUT) {
            QCC_LogError(status, ("Test::Run(): Event::Wait(): Failed"));
            break;
        }

        for (std::vector<qcc::Event*>::iterator i = signaledEvents.begin(); i != signaledEvents.end(); ++i) {
            if (*i == &stopEvent) {
                QCC_DbgPrintf(("Test::Run(): Stop event fired"));
                stopEvent.ResetEvent();
            } else if (*i == &timerEvent) {
                if (g_user) {
                    if (connectSent == false) {
                        connectSent = true;
                        ArdpConnRecord* conn;
                        ARDP_Connect(handle, sock, qcc::IPAddress(g_address), atoi(g_foreignport), ARDP_SEGMAX, ARDP_SEGBMAX,
                                     &conn, (uint8_t* )g_ajnConnString, strlen(g_ajnConnString) + 1, NULL);
                        continue;
                    }
                }
            } else {
                QCC_DbgPrintf(("Test::Run(): Socket event fired"));
                uint32_t ms;
                ARDP_Run(handle, sock, true, &ms);
            }
        }
    }
    ARDP_FreeHandle(handle);
    return 0;
}

int main(int argc, char** argv)
{
    printf("%s main()\n", argv[0]);

    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-l", argv[i])) {
            g_localport = argv[i + 1];
            i += 1;
        } else if (0 == strcmp("-f", argv[i])) {
            g_foreignport = argv[i + 1];
            i += 1;
        } else if (0 == strcmp("-a", argv[i])) {
            g_address = argv[i + 1];
            i += 1;
        } else if (0 == strcmp("-u", argv[i])) {
            g_user = true;
        } else {
            printf("Unknown option %s\n", argv[i]);
            exit(0);
        }
    }

    printf("g_user == %d.\n", g_user);
    printf("g_localport == %s\n", g_localport);
    printf("g_foreignport == %s\n", g_foreignport);
    printf("g_addresss == %s\n", g_address);

    signal(SIGINT, SigIntHandler);

    Test test;
    test.Start();

    while (g_interrupt == false) {
        qcc::Sleep(100);
    }

    test.Stop();
    test.Join();

    exit(0);
}