// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NBGameState.generated.h"

/**
 * 게임 전반의 상태를 보관하고 모든 클라이언트에 복제한다.
 */
UCLASS()
class NUMBERBASEBALL_API ANBGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	//	야구 게임 진행 여부 (서버에서 설정 → 클라이언트로 복제)
	bool IsGameOn() const { return bIsGameOn; }

	//	서버 권위 측에서만 호출한다.
	void SetGameOn(bool bInGameOn);

	//	정답 숫자 (서버에만 존재하며 클라이언트로 복제하지 않는다.)
	const FString& GetCorrectAnswer() const { return CorrectAnswer; }

	//	현재 턴의 제한시간 타이머를 시작/해제한다. (서버 권위 측에서만 호출)
	void StartTurnTimer(float Duration);
	void ClearTurnTimer();

	//	현재 턴 타이머가 동작 중인지 여부.
	bool IsTurnTimerActive() const { return TurnDuration > 0.f; }

	//	남은 턴 시간 비율 [0,1] (프로그레스바용). 서버/클라이언트 모두에서 동작한다.
	float GetRemainingTurnRatio() const;

	//	남은 턴 시간(초).
	float GetRemainingTurnSeconds() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated)
	bool bIsGameOn = false;

	//	현재 턴이 끝나는 서버 월드 시각(초). GetServerWorldTimeSeconds() 기준.
	UPROPERTY(Replicated)
	float TurnEndServerTime = 0.f;

	//	현재 턴의 전체 제한시간(초). 0이면 타이머 비활성.
	UPROPERTY(Replicated)
	float TurnDuration = 0.f;

	FString CorrectAnswer;
};
