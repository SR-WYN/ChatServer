#pragma once

class NodeHeartbeat
{
public:
    static void start();
    static void stop();

private:
    static void runLoop();
};
