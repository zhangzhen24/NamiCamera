// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class UNamiCameraFeature;

class FNamiCameraModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** 获取Feature缓存 */
	static TMap<TSubclassOf<UNamiCameraFeature>, TArray<UNamiCameraFeature*>>& GetFeatureCache();

	/** 从缓存获取或创建Feature实例 */
	static UNamiCameraFeature* GetOrCreateFeatureInstance(TSubclassOf<UNamiCameraFeature> FeatureClass);

	/** 回收Feature实例到缓存 */
	static void RecycleFeatureInstance(UNamiCameraFeature* FeatureInstance);

private:
	/** 全局Feature缓存 */
	static TMap<TSubclassOf<UNamiCameraFeature>, TArray<UNamiCameraFeature*>> FeatureCache;

	/** 缓存大小限制 */
	static const int32 MaxFeatureInstancesPerClass = 5;
};

