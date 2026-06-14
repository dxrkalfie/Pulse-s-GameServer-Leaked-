#pragma once
#include "pch.h"

class Configuration
{
public:
	static inline bool bIsLateGame = false;
	static inline bool bIsGameStarted = false; // DO NOT TOUCH
	static inline bool bIs1V1Map = true;

	static inline bool bIsProd = true; // Sends API requests
	static inline bool bIsRestarting = false; // DO NOT TOUCH

	static inline std::string Region = "EU";
};