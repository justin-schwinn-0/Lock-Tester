cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_CXX_STANDARD 17)

project(LockTester)

set(THREADS_PREFER_PTHREAD_FLAG ON)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -g -O1")

find_package(Threads REQUIRED)

set(Sources
    main.cpp
    Utils.cpp
    PetersonLock.cpp
    Gpl.cpp
    FilterBB.cpp
    FilterTB.cpp
    TournamentLock.cpp
    BakersLamportLock.cpp
    BakersTBLock.cpp
    FetchAndIncLock.cpp
    McsLock.cpp
    CnaLock.cpp
    CrmrRwLock.cpp
    MrwLock.cpp
    MrwLockOpt.cpp
)

set(Headers
    Utils.h
    PetersonLock.h
    Gpl.h
    FilterBB.h
    FilterTB.h
    Flag.h
    TournamentLock.h
    BakersLamportLock.h
    BakersTBLock.h
    FetchAndIncLock.h
    LockCommon.h
    McsLock.h
    CnaLock.h
    BaseRwLock.h
    CrmrRwLock.h
    MrwLock.h
    MrwLockOpt.h
)

add_executable(${PROJECT_NAME} ${Sources} ${Headers})
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
