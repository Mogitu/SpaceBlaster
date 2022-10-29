// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTiledLevelDev, Warning, All)

#define DEV_LOG(s) UE_LOG(LogTiledLevelDev, Warning, TEXT(s))
#define DEV_LOGF(s, ...) UE_LOG(LogTiledLevelDev, Warning, TEXT(s), __VA_ARGS__)
#define ERROR_LOG(s) UE_LOG(LogTiledLevelDev, Error, TEXT(s))