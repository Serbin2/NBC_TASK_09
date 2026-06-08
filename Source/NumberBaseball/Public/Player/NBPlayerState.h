// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NBPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class NUMBERBASEBALL_API ANBPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	
	void InitGame();
	
	int32 GetRemainChance() const { return RemainChance; };
	bool HasRemainChance() const { return RemainChance > 0; }
	bool SpendChance();

	//	현재 이 플레이어의 차례인지 여부.
	bool IsMyTurn() const { return bIsMyTurn; }

	//	서버 권위 측에서만 호출한다.
	void SetMyTurn(bool bInMyTurn);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	UPROPERTY(Replicated)
	int32 RemainChance = 3;

	UPROPERTY(Replicated)
	int32 MaxChance = 3;

	UPROPERTY(Replicated)
	bool bIsMyTurn = false;
};
