#include "Core/Logger.h"
#include "Core/Trace.h"
#include "Core/LogDefs.h"
#include "Params.h"
#include "ClusterManager.h"

int main()
{
    Logger::Instance().Start("");
    ClusterManager::Instace();
    Trace(SOURCE, "Cluster Manager initiated");
    ClusterManager::Instace().WaitForCompletion();
    return 0;
}
