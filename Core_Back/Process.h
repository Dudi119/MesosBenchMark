#pragma once
#include <vector>
#include <type_traits>
#include <unistd.h>
#include "Assert.h"
#include "Pipe.h"
#include "ChildProcess.h"

namespace core
{
    class Process {
    public:
        template<typename... Args>
        static ChildProcess SpawnChildProcess(const char* processPath, Args... args){
            ValidateArgsType<Args...>();

            std::unique_ptr<Pipe> stdOutPipe(new Pipe());
            std::unique_ptr<Pipe> stdErrorPipe(new Pipe());
            pid_t processID = fork();
            LINUX_VERIFY(processID != -1);
            if(processID == 0) { //child
                stdOutPipe->CloseReadDescriptor();
                dup2(stdOutPipe->GetWriteDescriptor(), Pipe::STD_OUT);
                stdErrorPipe->CloseReadDescriptor();
                dup2(stdErrorPipe->GetWriteDescriptor(), Pipe::STD_ERROR);
                execlp(processPath, args...);
                exit(0);
            }
            //Only parent will reach this part
            return ChildProcess(processID, stdOutPipe, stdErrorPipe);
        }

    private:
        template<typename... Args>
        static typename std::enable_if<sizeof...(Args) == 0>::type ValidateArgsType(){}
        template<typename First, typename... Args>
        static void ValidateArgsType(){
            static_assert(std::is_same<First, const char*>::value == true, "Format only supports c-type string as type");
            ValidateArgsType<Args...>();
        }
    };
}
