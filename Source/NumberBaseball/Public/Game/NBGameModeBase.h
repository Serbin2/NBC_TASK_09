// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/GameModeBase.h"
#include "NBGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class NUMBERBASEBALL_API ANBGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	ANBGameModeBase();

	virtual void Logout(AController* Exiting) override;

	void BroadcastChatMessage(const FString& SenderName, const FString& Message);
	void SendSystemMessageToPlayer(APlayerController* Target, const FString& Message, FLinearColor Color = FLinearColor::Yellow);
	void RequestGameOn(APlayerController* Requester);

	//	클라이언트가 검증한 추측 숫자를 받아 게임 로직을 처리한다.
	void SubmitNumber(APlayerController* Requester, const FString& Number);

private:
	//	추측 숫자를 정답과 비교해 스트라이크/볼 개수를 판정한다.
	void JudgeGuess(const FString& Answer, const FString& Guess, int32& OutStrikes, int32& OutBalls) const;

	//	턴 순서에서 해당 플레이어의 인덱스를 찾는다. (없으면 INDEX_NONE)
	int32 FindTurnIndex(const APlayerController* PC) const;

	//	모든 참가자의 턴 플래그를 끈다.
	void ClearAllTurnFlags();

	//	특정 플레이어에게 차례를 넘기고 모두에게 알린다.
	void GiveTurnTo(APlayerController* PC);

	//	SearchStartIndex부터 순회하며 남은 기회가 있는 다음 플레이어에게 차례를 넘긴다.
	//	(차례를 받을 플레이어가 아무도 없으면 false)
	bool TryAdvanceTurn(int32 SearchStartIndex);

	//	CurrentTurnIndex 다음 순서부터 차례를 넘긴다. 넘길 사람이 없으면 무승부로 종료한다.
	void AdvanceTurnFrom(int32 CurrentTurnIndex);

	//	현재 차례 플레이어가 제한시간 내에 도전하지 않았을 때 호출된다.
	void OnTurnTimeout();

	//	게임을 종료하고 턴 관련 상태를 정리한다.
	void EndGame();

	//	무승부 메시지와 함께 게임을 종료한다.
	void EndGameAsDraw();

	//	이번 게임에 참가한 플레이어의 턴 순서. (게임 시작 시 고정, 중간 참가 불가)
	TArray<TWeakObjectPtr<APlayerController>> TurnOrder;

	//	현재 차례인 플레이어.
	TWeakObjectPtr<APlayerController> CurrentTurnPC;

	//	각 턴의 제한시간(초). 시간 내 도전하지 않으면 기회를 차감하고 차례를 넘긴다.
	UPROPERTY(EditDefaultsOnly, Category="NumberGame|Turn")
	float TurnTimeLimit = 120.f;

	//	현재 턴의 제한시간 타이머.
	FTimerHandle TurnTimerHandle;
};
