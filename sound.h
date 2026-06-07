#ifndef __SOUND_H
#define __SOUND_H

#include <windows.h>
#include <mmsystem.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#pragma comment(lib, "winmm.lib")

void InitSoundSystem();
void ShutdownSoundSystem();
void PlayBGM(const std::wstring& fileName, int volume);
void StopBGM();
void PlaySoundEffect(const std::wstring& fileName, int volume);
void StopSoundEffect();
void SetBGMVolume(int volume);
void SetSoundEffectVolume(int volume);
void PlayEffectThread(std::wstring fileName);


#endif /* __SOUND_H */