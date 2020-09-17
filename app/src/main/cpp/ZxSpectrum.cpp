//
// Created by Игорь on 17.06.2018.
//

#include <thread>
#include <string>
#include <ctime>

#include "ZxSpectrum.h"

static uint8_t readFromMemory(void* userData, uint16_t address) {
    auto zxSpectrum = static_cast<ZxSpectrum*>(userData);
    uint8_t* memory = zxSpectrum->memoryArray();
    return memory[address];
}

static void writeToMemory(void* userData, uint16_t address, uint8_t data) {
    if (address < 0x4000) {
        return;
    }
    auto zxSpectrum = static_cast<ZxSpectrum*>(userData);
    uint8_t* memory = zxSpectrum->memoryArray();
    memory[address] = data;
}

static uint8_t readFromPort(void* userData, uint16_t port) {
    auto zxSpectrum = static_cast<ZxSpectrum*>(userData);
    uint8_t* io = zxSpectrum->portsArray();
    return io[port];
}

static void writeToPort(void* userData, uint16_t port, uint8_t data) {
    auto zxSpectrum = static_cast<ZxSpectrum*>(userData);
    uint8_t* io = zxSpectrum->portsArray();
    io[port & 0x00ff] = data;
}

static zuint32 readIntData(void* userData) {
    throw std::runtime_error("Not implemented");
}

static void haltedCallback(void* userData, zboolean isHalted) {
    // do nothing
}

ZxSpectrum::ZxSpectrum(int sampleRate, int bufferSize, BeeperCallback beeperCallback, std::string logFilePath) :
        m_isRunning(true),
        m_shouldInterrupt(0),
//		m_prevTstates(0),
        m_totalInstructionsCounter(0),
        m_exceededInstructionsCounter(0),
        m_interruptsCount(0)
{
    m_keyboard = new Keyboard(m_portsArray);
    m_beeper = new Beeper(m_portsArray, sampleRate, bufferSize, beeperCallback);
    /*m_cpu = new Z80(readFromMemory, writeToMemory, readFromPort, writeToPort);
    m_cpu->setMemoryUserData(m_memoryArray);
    m_cpu->setIoUserData(m_portsArray);*/

    m_cpu.context = this;
    m_cpu.read = readFromMemory;
    m_cpu.write = writeToMemory;
    m_cpu.in = readFromPort;
    m_cpu.out = writeToPort;
    m_cpu.int_data = readIntData;
    m_cpu.halt = haltedCallback;

    z80_power(&m_cpu, true);

    reset();
}

ZxSpectrum::~ZxSpectrum() {
    //delete m_cpu;
    delete m_beeper;
    delete m_keyboard;
}

void ZxSpectrum::soundLoop() {
    m_beeper->loop();
}

void ZxSpectrum::loop() {
    m_isRunning = true;
    while (m_isRunning) {
        //const uint64_t startTstates = m_cpu->tStates();
        //clock_gettime(CLOCK_MONOTONIC, &m_startTimestamp);

        bool shouldInterrupt = m_shouldInterrupt > 0;
        if (shouldInterrupt) {
            m_interruptsCount++;
            m_shouldInterrupt--;

            //m_cpu->interrupt(0);
            /*z80_int(&m_cpu, false);
            z80_int(&m_cpu, true);*/
        }
        z80_int(&m_cpu, shouldInterrupt);

        z80_run(&m_cpu, 30);

        //uint64_t currentTstates = m_cpu->tStates();
        //clock_gettime(CLOCK_MONOTONIC, &m_currentTimestamp);

        /*long expectedTime = m_cpu.cycles * CPU_CLOCK_PERIOD;
        long actualTime = uint64_t(m_currentTimestamp.tv_nsec - m_startTimestamp.tv_nsec);*/

        /*if (expectedTime > actualTime) {
            timespec timeToSleep;
            timeToSleep.tv_sec = 0;
            timeToSleep.tv_nsec = 1;//expectedTime - actualTime;
            clock_nanosleep(CLOCK_MONOTONIC, 0, &timeToSleep, nullptr);
            while (expectedTime > actualTime) {
                clock_gettime(CLOCK_MONOTONIC, &m_currentTimestamp);
                actualTime = uint64_t(m_currentTimestamp.tv_nsec - m_startTimestamp.tv_nsec);
            }
        } else {
            m_exceededInstructionsCounter++;
        }*/

        //m_totalInstructionsCounter++;

        m_instructionsCount++;
    }
}

void ZxSpectrum::quit() {
    m_beeper->stop();
    m_isRunning = false;
}

void ZxSpectrum::reset() {
    z80_reset(&m_cpu);
}

void ZxSpectrum::verticalRefresh() {
    if (m_isRunning) {
        m_shouldInterrupt++;
    }
}

uint32_t ZxSpectrum::interruptsCount() {
    uint32_t count = m_interruptsCount;
    m_interruptsCount = 0;
    return count;
}

uint32_t ZxSpectrum::instructionsCount() {
    uint32_t count = m_instructionsCount;
    m_instructionsCount = 0;
    return count;
}

void ZxSpectrum::onKeyPressed(const int keyCode) {
    m_keyboard->onKeyPressed(keyCode);
}

void ZxSpectrum::onKeyReleased(const int keyCode) {
    m_keyboard->onKeyReleased(keyCode);
}
