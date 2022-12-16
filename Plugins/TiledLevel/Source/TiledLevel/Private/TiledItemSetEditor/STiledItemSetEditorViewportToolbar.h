// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "SViewportToolBar.h"
#include "Widgets/Input/SSpinBox.h"

class STiledItemSetEditorViewportToolbar : public SViewportToolBar
{
public:
	SLATE_BEGIN_ARGS(STiledItemSetEditorViewportToolbar) {}
		SLATE_ARGUMENT(TWeakPtr<class STiledItemSetEditorViewport>, EditorViewport)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	
	FText GetCameraSpeedLabel() const;
	TSharedRef<SWidget> FillCameraSpeedMenu();
	TSharedPtr<class SSlider> CamSpeedSlider;
	float GetCamSpeedSliderPosition() const;
	void OnSetCamSpeed(float NewValue);
	TSharedPtr<SSpinBox<float>> CamSpeedScalarBox;
	float GetCamSpeedScalarBoxValue() const;
	void OnSetCamSpeedScalarBoxValue(float NewValue);
	
	// TSharedRef<SWidget> GeneratePreviewMenu(); // For future updates?
	FText GetCameraMenuLabel() const;
	const FSlateBrush* GetCameraMenuLabelIcon() const;
	TSharedRef<SWidget> GenerateCameraMenu();

	FText GetViewMenuLabel() const;
	const FSlateBrush* GetViewMenuLabelIcon() const;
	TSharedRef<SWidget> GenerateViewMenu();

private:
	TWeakPtr<STiledItemSetEditorViewport> EditorViewport;
};
