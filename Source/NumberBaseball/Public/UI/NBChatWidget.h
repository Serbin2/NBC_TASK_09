// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NBChatWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UImage;
/**
 *
 */
UCLASS()
class NUMBERBASEBALL_API UNBChatWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual auto NativeConstruct() -> void override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void AddMessage(const FString& MessageOwner, const FString& MessageText, FLinearColor Color = FLinearColor::Black);

protected:
	UFUNCTION()
	void OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	//	매 틱 턴 타이머 프로그레스바와 내 턴 표시 이미지를 갱신한다.
	void UpdateTurnUI();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UEditableTextBox> EditableTextBox_ChatInput;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UVerticalBox> VerticalBox_MessageContainer;

	//	현재 턴의 남은 제한시간을 표시하는 프로그레스바.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar_TurnTimer;

	//	내 차례일 때만 보이는 이미지.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_MyTurn;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> ChatMessageQueue;
	
	UPROPERTY(EditDefaultsOnly, Category="NumberGame|Chat")
	int32 MaxMessageCount = 20;
	
};
