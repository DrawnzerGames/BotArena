// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/BotController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISenseConfig.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "AICharacter.h"
#include "Kismet/KismetMathLibrary.h"


ABotController::ABotController()
{
	//Keys init
	BlackboardKey_MoveLocation = FName("MoveLocation");
	BlackboardKey_SelectedTarget = FName("SelectedTarget");

	//Create the AI perception component
	if (!GetPerceptionComponent())
	{
		PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(FName("PawnSensingComp"));

		if (PerceptionComp)
		{
			SetPerceptionComponent(*PerceptionComp);
		}
	}
	
}

void ABotController::SelectTarget(const TArray<AActor*>& TargetList)
{
	ensure(GetBlackboardComponent());

	AAICharacter* ControlledCharacter = Cast<AAICharacter>(GetCharacter());

	if (!ControlledCharacter || TargetList.Num()<=0) return;

	//Search for the closest target
	float ClosestDistance = 99999.f;
	AAICharacter* SelectedTarget = nullptr;
	FVector CharacterLocation = ControlledCharacter->GetActorLocation();

	for (int32 TargetIndex = 0; TargetIndex < TargetList.Num(); TargetIndex++)
	{
		//Only choose a target from Bots
		AAICharacter* Bot = Cast<AAICharacter>(TargetList[TargetIndex]);
		if (Bot)
		{
			if (ControlledCharacter->IsHostile(*Bot))
			{
				if ((Bot->GetActorLocation() - CharacterLocation).Size() < ClosestDistance)
				{
					ClosestDistance = (Bot->GetActorLocation() - CharacterLocation).Size();
					SelectedTarget = Bot;
				}
			}
		}
	}
	GLog->Log("selected target from sensed actors!");
	GetBlackboardComponent()->SetValueAsObject(BlackboardKey_SelectedTarget, SelectedTarget);
}

void ABotController::OnPerceptionUpdated(const TArray<AActor*>& SensedActors)
{
	GLog->Log("On Perception updated!");
	SelectTarget(SensedActors);

}

void ABotController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!BTAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid BT Asset"));
		return;
	}

	RunBehaviorTree(BTAsset);
	if (GetPerceptionComponent())
	{
		GetPerceptionComponent()->OnPerceptionUpdated.AddDynamic(this, &ABotController::OnPerceptionUpdated);
		UAISenseConfig_Sight* SightConfig = Cast<UAISenseConfig_Sight>(GetPerceptionComponent()->GetSenseConfig(UAISense::GetSenseID<UAISense_Sight>()));
		if (SightConfig) 
		{
			GLog->Log("valid sight cfg");
			SightConfig->DetectionByAffiliation.bDetectEnemies = true;
			SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
			SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		}
	}
}

void ABotController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UBlackboardComponent* BlackBoardComp = GetBlackboardComponent();
	
	if (UObject* SelectedTarget = BlackBoardComp->GetValueAsObject(BlackboardKey_SelectedTarget))
	{
		AActor* PossesedActor = GetCharacter();
		AActor* TargetToFace = Cast<AActor>(SelectedTarget);
		if (PossesedActor && TargetToFace)
		{
			PossesedActor->SetActorRotation(UKismetMathLibrary::FindLookAtRotation(PossesedActor->GetActorLocation(), TargetToFace->GetActorLocation()));
		}
	}

}

//void ABotController::BeginPlay()
//{
//	Super::BeginPlay();
//
//	if (!BTAsset)
//	{
//		GLog->Log("invalid bt"); //warning here
//		return;
//	}
//
//}
