// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "SViewportToolBar.h"
#include "Widgets/Input/SSpinBox.h"

class TILEDLEVEL_API STiledLevelEditorViewportToolbar : public SViewportToolBar
{
public:
	SLATE_BEGIN_ARGS(STiledLevelEditorViewportToolbar) {}
		SLATE_ARGUMENT(TWeakPtr<class STiledLevelEditorViewport>, EditorViewport)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	
	FText GetCameraSpeedLabel() const;
	TSharedRef<SWidget> FillCameraSpeedMenu();
	float GetCamSpeedSliderPosition() const;
	void OnSetCamSpeed(float NewValue);
	float GetCamSpeedScalarBoxValue() const;
	void OnSetCamSpeedScalarBoxValue(float NewValue);
	
	// TSharedRef<SWidget> GenerateShowMenu();
	FText GetCameraMenuLabel() const;
	const FSlateBrush* GetCameraMenuLabelIcon() const;
	TSharedRef<SWidget> GenerateCameraMenu();

	FText GetViewMenuLabel() const;
	const FSlateBrush* GetViewMenuLabelIcon() const;
	TSharedRef<SWidget> GenerateViewMenu();

private:
	TSharedPtr<class SSlider> CamSpeedSlider;
	TSharedPtr<SSpinBox<float>> CamSpeedScalarBox;
	TWeakPtr<STiledLevelEditorViewport> EditorViewport;

};
