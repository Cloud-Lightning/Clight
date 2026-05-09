#pragma once

extern "C" {
#include "lwip/apps/httpd.h"
}

#include "EthLwipStack.hpp"

class EthLwipHttpServer {
public:
    explicit EthLwipHttpServer(Eth eth = Eth(EthId::Port0))
        : stack_(eth)
    {
    }

    Status start()
    {
        return stack_.start(&httpd_init);
    }

    void poll()
    {
        stack_.poll();
    }

    bool started() const { return stack_.started(); }

    Status linkState(EthLinkState &state) const
    {
        return stack_.linkState(state);
    }

    EthLwipStack &stack() { return stack_; }
    const EthLwipStack &stack() const { return stack_; }

    Eth &eth() { return stack_.eth(); }
    const Eth &eth() const { return stack_.eth(); }

private:
    EthLwipStack stack_;
};
