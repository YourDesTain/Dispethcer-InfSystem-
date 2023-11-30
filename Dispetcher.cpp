#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <tchar.h>
#include <dxgi.h>
#include <psapi.h>
#include <vector>
#include <algorithm>

using namespace std;

void getProcessorInfo() { //получение информации о процессоре
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    cout << "Процессор: " << sysInfo.dwNumberOfProcessors << " ядер(а)" << endl;
    cout << "Тип процессора: " << sysInfo.dwProcessorType << endl;

    HKEY hKey;
    const char* keyPath = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
    const char* valueName = "ProcessorNameString";

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        DWORD dataSize = 0;
        if (RegQueryValueExA(hKey, valueName, nullptr, nullptr, nullptr, &dataSize) == ERROR_SUCCESS) {
            char* processorName = new char[dataSize];
            if (RegQueryValueExA(hKey, valueName, nullptr, nullptr, reinterpret_cast<BYTE*>(processorName), &dataSize) == ERROR_SUCCESS) {
                cout << "Название процессора: " << processorName << endl;
            }
            delete[] processorName;
        }
        RegCloseKey(hKey);
    }
}

void getMemoryInfo() { //получение информации о памяти(ОЗУ)
    MEMORYSTATUSEX memoryInfo;
    memoryInfo.dwLength = sizeof(memoryInfo);
    GlobalMemoryStatusEx(&memoryInfo);
    cout << "ОЗУ: " << memoryInfo.ullTotalPhys / (1024 * 1024) << " МБ" << endl;
}

void getDiskInfo() { //получение информации о диске
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    GetDiskFreeSpaceEx(_T("C:\\"), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes);

    cout << "HDD: " << totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024) << " ГБ" << endl;
}

void getGPUInfo() {  //получение информации о GPU
    IDXGIFactory1* pFactory = nullptr;

    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory))) {  //создание CreateDXGIFactory
        cout << "Ошибка при получении информации о GPU" << endl;
        return;
    }
    IDXGIAdapter1* pAdapter = nullptr;  //инициализация адаптера
    for (UINT i = 0; pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        pAdapter->GetDesc1(&desc);

        wcout << L"GPU: " << desc.Description << endl;
        wcout << L"Видеопамять: " << desc.DedicatedVideoMemory / (1024 * 1024) << L" МБ" << endl;     
        pAdapter->Release();
    }

    pFactory->Release();
}

void displayHardwareInfo() {  //вывод характеристик ПК
    cout << "---- Характеристики ПК ----" << endl;
    getProcessorInfo();
    getMemoryInfo();
    getDiskInfo();
    getGPUInfo();
}

void displayCpuLoad() {
    FILETIME idleTime, kernelTime, userTime;
       if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) 
       { 
            //конвертирование времени в 64-битное число
            ULONGLONG idleTime64 = ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
            ULONGLONG kernelTime64 = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
            ULONGLONG userTime64 = ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;

            ULONGLONG totalTime = kernelTime64 + userTime64;   //общее количество времени процессора

            double cpuLoad = 100.0 - (static_cast<double>(idleTime64) / totalTime * 100.0);  //вычисление загрузки процессора в процентах

            cout << "Нагрузка на процессор: " << cpuLoad << "%" << endl;
       }
       else {
            cerr << "Ошибка при получении информации о нагрузке на процессор" << endl;              
       }
}

void displayMemoryUsage() { //выделение оперативной памяти
    MEMORYSTATUSEX memoryInfo;
    memoryInfo.dwLength = sizeof(memoryInfo);

    if (GlobalMemoryStatusEx(&memoryInfo)) {
        cout << "Использовано: " << (memoryInfo.ullTotalPhys - memoryInfo.ullAvailPhys) / (1024 * 1024) << " МБ" << endl;
        cout << "Свободно: " << memoryInfo.ullAvailPhys / (1024 * 1024) << " МБ" << endl;
    }
    else {
        cerr << "Ошибка при получении информации об использовании оперативной памяти" << endl;
    }
}

void displayDiskSpace() { //емкость диска
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;

    if (GetDiskFreeSpaceEx(_T("C:\\"), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        cout << "Занято: " << (totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart) / (1024 * 1024 * 1024) << " ГБ" << endl;
        cout << "Свободно: " << totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024) << " ГБ" << endl;
    }
    else {
        cerr << "Ошибка при получении информации о диске" << endl;
    }
    if (GetDiskFreeSpaceEx(_T("F:\\"), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        cout << "Занято: " << (totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart) / (1024 * 1024 * 1024) << " ГБ" << endl;
        cout << "Свободно: " << totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024) << " ГБ" << endl;
    }
    else {
        cerr << "Ошибка при получении информации о диске" << endl;
    }
}


struct ProcessInfo {    //структура для хранения информации о процессе
    DWORD processId;
    wstring processName;
    SIZE_T memoryUsage;
};

vector<ProcessInfo> getProcessList() { //получение списка процессов
    vector<ProcessInfo> processes;
    DWORD processesArray[1024], bytesNeeded;

    if (EnumProcesses(processesArray, sizeof(processesArray), &bytesNeeded)) {
        DWORD numProcesses = bytesNeeded / sizeof(DWORD);

        for (DWORD i = 0; i < numProcesses; ++i) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processesArray[i]);
            if (hProcess != NULL) {
                ProcessInfo processInfo;
                processInfo.processId = processesArray[i];

                TCHAR szProcessName[MAX_PATH];  //получение имени процесса
                if (GetModuleBaseName(hProcess, NULL, szProcessName, sizeof(szProcessName) / sizeof(TCHAR))) {
                    processInfo.processName = szProcessName;
                }
                else {
                    processInfo.processName = L"Unknown";
                }

                //получение информации о памяти процесса
                PROCESS_MEMORY_COUNTERS_EX pmc;
                if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                    processInfo.memoryUsage = pmc.WorkingSetSize;
                }
                else {
                    processInfo.memoryUsage = 0;
                }

                processes.push_back(processInfo);

                CloseHandle(hProcess);
            }
        }
    }

    return processes;
}

bool compareByMemoryUsage(const ProcessInfo& a, const ProcessInfo& b) {   //сравнение двух процессов по использованию памяти
    return a.memoryUsage > b.memoryUsage;
}

void displayProcessList(bool sortByMemory) {   //вывод списка процессов с сортировкой
    vector<ProcessInfo> processes = getProcessList();

    if (sortByMemory) {
        sort(processes.begin(), processes.end(), compareByMemoryUsage);
        cout << "\n---- Список процессов(сортированный по использованию памяти) ----" << endl;
    }
    else {
        cout << "\n---- Список процессов ----" << endl;
    }

    for (const auto& process : processes) {
        wcout << L"ID: " << process.processId << L"\tИмя: " << process.processName << L"\tПамять: " << process.memoryUsage / (1024 * 1024) << " МБ" << endl;
    }
}

int main() {
    setlocale(LC_ALL, "russian");

    displayHardwareInfo(); //вывод информации о характеристиках ПК

    cout << "\n---- Нагрузка на процессор ----" << endl;
    displayCpuLoad(); //вывод нагрузки на процессор

    cout << "\n---- Использование оперативной памяти ----" << endl;
    displayMemoryUsage();  //вывод информации об использовании ОЗУ

    cout << "\n---- Емкость диска (HDD/SSD) ----" << endl;
    displayDiskSpace(); //вывод емкости дисков

    displayProcessList(false);  //вывод списка процессов без сортировки

    displayProcessList(true);  //отсотированный список процессов

    return 0;
}
