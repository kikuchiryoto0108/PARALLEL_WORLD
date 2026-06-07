#include "sound.h"
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>

static std::thread bgmThread;
static std::vector<std::thread> effectThreads;
static std::mutex soundMutex;
static std::atomic<bool> isPlayingBGM{ false };
static std::atomic<bool> stopBGMThread{ false };
static int bgmVolume = 100;  // BGMの音量（0-100）
static int soundEffectVolume = 100;  // 効果音の音量（0-100）
static const int maxEffectThreads = 10;  // 効果音を再生するスレッドの最大数

// 効果音再生キューと条件変数
static std::queue<std::wstring> effectQueue;
static std::condition_variable effectCV;
static std::atomic<bool> stopEffectThread{ false };
static std::thread effectManagerThread;

void InitSoundSystem() {
    // 必要な初期化処理があればここに追加
    std::cout << "Sound system initialized." << std::endl;

    // 効果音管理スレッドの開始
    stopEffectThread = false;
    effectManagerThread = std::thread([]() {
        while (!stopEffectThread) {
            std::unique_lock<std::mutex> lock(soundMutex);
            effectCV.wait(lock, [] { return !effectQueue.empty() || stopEffectThread; });

            while (!effectQueue.empty()) {
                if (effectThreads.size() >= maxEffectThreads) {
                    // 最も古いスレッドを削除
                    if (effectThreads.front().joinable()) {
                        effectThreads.front().join();
                    }
                    effectThreads.erase(effectThreads.begin());
                }

                std::wstring fileName = effectQueue.front();
                effectQueue.pop();

                // 効果音再生スレッドを作成して追加
                effectThreads.emplace_back(PlayEffectThread, fileName);
                effectThreads.back().detach();
            }
        }
        });
}

void ShutdownSoundSystem() {
    StopBGM();
    StopSoundEffect();
    stopEffectThread = true;
    effectCV.notify_all();
    if (effectManagerThread.joinable()) {
        effectManagerThread.join();
    }
    std::cout << "Sound system shutdown." << std::endl;
}

void PlayBGMThread(std::wstring fileName) {
    std::wstring command = L"open \"" + fileName + L"\" type mpegvideo alias bgm";
    mciSendStringW(command.c_str(), NULL, 0, NULL);
    command = L"play bgm repeat";
    mciSendStringW(command.c_str(), NULL, 0, NULL);
    while (!stopBGMThread) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    mciSendStringW(L"stop bgm", NULL, 0, NULL);
    mciSendStringW(L"close bgm", NULL, 0, NULL);
}

void PlayBGM(const std::wstring& fileName, int volume) {
    std::lock_guard<std::mutex> lock(soundMutex);

    if (isPlayingBGM) {
        return; // 既にBGMが再生中なら何もしない
    }

    stopBGMThread = false;
    SetBGMVolume(volume);
    isPlayingBGM = true;

    // BGM再生スレッドを作成して追加
    bgmThread = std::thread(PlayBGMThread, fileName);
}

void StopBGM() {
    std::lock_guard<std::mutex> lock(soundMutex);

    if (isPlayingBGM) {
        stopBGMThread = true;
        if (bgmThread.joinable()) {
            bgmThread.join();
        }
        isPlayingBGM = false;
    }
}

void PlayEffectThread(std::wstring fileName) {
    std::wstring command = L"open \"" + fileName + L"\" type mpegvideo alias effect";
    mciSendStringW(command.c_str(), NULL, 0, NULL);
    command = L"play effect from 0";
    mciSendStringW(command.c_str(), NULL, 0, NULL);
    std::this_thread::sleep_for(std::chrono::seconds(5)); // 適切な待機時間を設定
    mciSendStringW(L"stop effect", NULL, 0, NULL);
    mciSendStringW(L"close effect", NULL, 0, NULL);
}

void PlaySoundEffect(const std::wstring& fileName, int volume) {
    std::lock_guard<std::mutex> lock(soundMutex);

    SetSoundEffectVolume(volume);

    // 効果音再生キューに追加
    effectQueue.push(fileName);
    effectCV.notify_one();
}

void StopSoundEffect() {
    std::lock_guard<std::mutex> lock(soundMutex);

    for (std::thread& t : effectThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
    effectThreads.clear();
}

void SetBGMVolume(int volume) {
    bgmVolume = volume;
    std::wstring command = L"setaudio bgm volume to " + std::to_wstring((bgmVolume * 1000) / 100);
    mciSendStringW(command.c_str(), NULL, 0, NULL);
}

void SetSoundEffectVolume(int volume) {
    soundEffectVolume = volume;
    std::wstring command = L"setaudio effect volume to " + std::to_wstring((soundEffectVolume * 1000) / 100);
    mciSendStringW(command.c_str(), NULL, 0, NULL);
}