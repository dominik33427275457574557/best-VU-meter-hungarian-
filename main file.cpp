#include <iostream>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <functiondiscoverykeys_devpkey.h>
#include <thread>
#include <chrono>
#include <comdef.h>
#include <cmath> // log10f

#pragma comment(lib, "Ole32.lib")

// Függvény előre deklarálása
float calibrateDb(float peakValue);

// Kiír egy egyszerű VU métert konzolra dB értékkel és csíkkal
void printVolumeBarAndDb(float db) {
    const int barLength = 50;
    int filled = static_cast<int>((db + 60) / 60 * barLength); // db -60 ... 0 tartomány, skálázva
    if (filled > barLength) filled = barLength;
    if (filled < 0) filled = 0;

    std::cout << "dB: " << db << " [";
    for (int i = 0; i < filled; ++i) std::cout << '#';
    for (int i = filled; i < barLength; ++i) std::cout << '-';
    std::cout << "]\r" << std::flush;
}

int main() {
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::cerr << "COM init failed\n";
        return -1;
    }

    IMMDeviceEnumerator* pEnumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pEnumerator));
    if (FAILED(hr)) {
        std::cerr << "Failed to create device enumerator\n";
        CoUninitialize();
        return -1;
    }

    IMMDevice* pDevice = nullptr;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        std::cerr << "Failed to get default audio endpoint\n";
        pEnumerator->Release();
        CoUninitialize();
        return -1;
    }

    IAudioMeterInformation* pMeterInfo = nullptr;
    hr = pDevice->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, nullptr, (void**)&pMeterInfo);
    if (FAILED(hr)) {
        std::cerr << "Failed to get audio meter information\n";
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return -1;
    }

    std::cout << "Legjobb VU Meter!\n";

    while (true) {
        float peakValue = 0.0f;
        hr = pMeterInfo->GetPeakValue(&peakValue);
        if (FAILED(hr)) {
            std::cerr << "Failed to get peak value\n";
            break;
        }

        float db = calibrateDb(peakValue);

        printVolumeBarAndDb(db);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    pMeterInfo->Release();
    pDevice->Release();
    pEnumerator->Release();
    CoUninitialize();

    return 0;
}

float calibrateDb(float peakValue) {
    if (peakValue < 0.001f) {
        return -60.0f;  // zajküszöb alatt nulla szint (-60 dB)
    }
    float db = 20.0f * log10f(peakValue);

    // Offset hozzáadása a kalibrációhoz
    db += 2.0f;

    // Max érték 0 dB legyen
    if (db > 0.0f) db = 0.0f;

    return db;
}
