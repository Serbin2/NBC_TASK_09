// Fill out your copyright notice in the Description page of Project Settings.


#include "NumberBaseball/Public/UI/NBChatWidget.h"

#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "NumberBaseball/Public/Game/NBGameState.h"
#include "NumberBaseball/Public/Player/NBPlayerController.h"
#include "NumberBaseball/Public/Player/NBPlayerState.h"

void UNBChatWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (!EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &UNBChatWidget::OnChatInputTextCommitted))
	{
		EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &UNBChatWidget::OnChatInputTextCommitted);
	}
}

void UNBChatWidget::NativeDestruct()
{
	if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &UNBChatWidget::OnChatInputTextCommitted))
	{
		EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &UNBChatWidget::OnChatInputTextCommitted);
	}
	
	Super::NativeDestruct();
}

void UNBChatWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateTurnUI();
}

void UNBChatWidget::UpdateTurnUI()
{
	//	턴 제한시간 → 프로그레스바.
	if (ProgressBar_TurnTimer)
	{
		float Ratio = 0.f;
		if (const UWorld* World = GetWorld())
		{
			if (const ANBGameState* NBGameState = World->GetGameState<ANBGameState>())
			{
				Ratio = NBGameState->GetRemainingTurnRatio();
			}
		}
		ProgressBar_TurnTimer->SetPercent(Ratio);
	}

	//	내 차례 여부 → 이미지 표시/숨김.
	if (Image_MyTurn)
	{
		const ANBPlayerState* NBPlayerState = GetOwningPlayerState<ANBPlayerState>();
		const bool bMyTurn = IsValid(NBPlayerState) && NBPlayerState->IsMyTurn();
		Image_MyTurn->SetVisibility(bMyTurn ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UNBChatWidget::AddMessage(const FString& MessageOwner, const FString& MessageText, FLinearColor Color)
{
	if (!VerticalBox_MessageContainer)	return;
	
	if (ChatMessageQueue.Num() >= MaxMessageCount)
	{
		UTextBlock* Oldest = ChatMessageQueue[0];
		VerticalBox_MessageContainer->RemoveChild(Oldest);
		ChatMessageQueue.RemoveAt(0);
	}
	
	UTextBlock* NewTextBlock = NewObject<UTextBlock>(this);
	NewTextBlock->SetText(FText::FromString(FString::Printf(TEXT("[%s] : %s"), *MessageOwner, *MessageText)));
	
	NewTextBlock->SetColorAndOpacity(Color);
	
	UVerticalBoxSlot* NewSlot = VerticalBox_MessageContainer->AddChildToVerticalBox(NewTextBlock);
	NewSlot->SetPadding(FMargin(4.f, 2.f));
	
	ChatMessageQueue.Add(NewTextBlock);
}

void UNBChatWidget::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter)	return;
	if (Text.IsEmpty())	return;

	ANBPlayerController* OwningNBPlayerController = Cast<ANBPlayerController>(GetOwningPlayer());
	if (!IsValid(OwningNBPlayerController))	return;

	OwningNBPlayerController->SendChatMessage(Text.ToString());

	EditableTextBox_ChatInput->SetText(FText());
}
