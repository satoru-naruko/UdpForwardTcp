#ifndef UDP_TO_TCP_FORWARDER_LOG_H
#define UDP_TO_TCP_FORWARDER_LOG_H

#pragma once

#include <cstdio>

#define ENABLE_UDP_TO_TCP_FORWARDER_LOG

#ifdef ENABLE_UDP_TO_TCP_FORWARDER_LOG
#define UDP_TO_TCP_FORWARDER_LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define UDP_TO_TCP_FORWARDER_LOG(fmt, ...) // No logging
#endif

#endif // UDP_TO_TCP_FORWARDER_LOG_H
