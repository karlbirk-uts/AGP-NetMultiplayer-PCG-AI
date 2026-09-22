// Fill out your copyright notice in the Description page of Project Settings.


#include "ConeCharacter.h"

// Sets default values
AConeCharacter::AConeCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AConeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Single-player GameMode's PlayerController
	if (TObjectPtr<APlayerController> PlayerController =
		Cast<APlayerController>
		(GetController()))
	{
		// If possessed by ULocalPlayer, try to use their EIS
		TObjectPtr<UEnhancedInputLocalPlayerSubsystem> Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>
			(PlayerController->GetLocalPlayer());

		if (IsValid(Subsystem))
		{
			// ULocalPlayer's EIS to use our IMC
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}
}

// Called every frame
void AConeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AConeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (TObjectPtr<UEnhancedInputComponent> Input =
	Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Bind UInputAction MoveAction axes events to Move()
		Input->BindAction(
			MoveAction,
			ETriggerEvent::Triggered,
			this,
			&AConeCharacter::Move);

		// Bind UInputAction LookAction axes events to Look()
		Input->BindAction(
			LookAction,
			ETriggerEvent::Triggered,
			this,
			&AConeCharacter::Look);


		// Bind UInputAction JumpAction bool events to StartJumping()
		Input->BindAction(
			JumpAction,
			ETriggerEvent::Started,
			this,
			&AConeCharacter::StartJumping);

		// Reportedly, best practice for keydown bool event jumping 
		// is to StopJumping() immediately afterwards
		Input->BindAction(
			JumpAction,
			ETriggerEvent::Completed,
			this,
			&AConeCharacter::EndJumping);
	}


	
	
	
}


void AConeCharacter::Move(const FInputActionValue& Value)
{
	/**
	In this Unreal Engine 5.6 project, default GameMode is
	"AGameModeBase", for local-players only.
	
	The map/level TestingMap's World Settings do not 
	override the project's default GameMode.
	
	This class is used as BP_PlayerCharacter, with mapped variables:
	UInputAction MoveAction to IA_Move
	UInputAction LookAction to IA_Look
	UInputAction JumpAction to IA_Jump

	The level's hand-placed instance of BP_PlayerCharacter
	AutoPossessPlayer is set to EAutoReceiveInput::Player0

	Upon playing the level, GameMode instantiates.
	GameMode spawns all actors in level, including BP_PlayerCharacter,
	possessed by Player0.

	Possession (by calling SetupPlayerInputComponent method) asserts 
	which UInputActions event binds will map to which class methods.

	Then PlayerController (Player0 EIS) is set to use Project's IMC,
	permitting player input listeners during gameplay.
	
	Player presses WSAD keys and the IA_Move axes information is parsed
	*/

	// Get IA_Move axes values for forward+- and right+- (strafing)
	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FVector ForwardVector = GetActorForwardVector();
	const FVector RightVector = GetActorRightVector();

	// Add axes values to current movement, with APawn::AddMovementInput
	// Note: This player movement is not normalised/clamped, 
	// behaving like Golden Eye N64.
	AddMovementInput(ForwardVector, MovementVector.X);
	AddMovementInput(RightVector, MovementVector.Y);
}

void AConeCharacter::Look(const FInputActionValue& Value)
{
	// Get IA_Look axes values
	const FVector2D LookVector = Value.Get<FVector2D>();

	// Add to / subtract from APlayerController InputYawScale and InputPitchScale
	AddControllerYawInput(LookVector.X * LookSensitivity);
	AddControllerPitchInput(LookVector.Y * LookSensitivity);
}

void AConeCharacter::StartJumping(const FInputActionValue& Value)
{
	// ACharacter::Jump() method built in (conditions e.g. bIsWalking)
	// Here I may insert logic that determines whether mid-air
	// jumping is allowed, or implement rocket boots
	Jump();
}

void AConeCharacter::EndJumping(const FInputActionValue& Value)
{
	// ACharacter::StopJumping() method built in
	StopJumping();
}

/**
* ULocalPlayer
* "Each player that is active on the current client/listen server 
* has a LocalPlayer. It stays active across maps, and there may 
* be several spawned in the case of splitscreen/coop. 
* There will be 0 spawned on dedicated servers."
*/

/**
 * GOOGLE: 
 * Q: "unreal engine 5.6 c++ can ACharacter call 
 * SetupPlayerInputComponent on UEnhancedInputComponent 
 * without casting, perhaps using an usinginterface?"
 * 
 * A: "No... The base UInputComponent class does not contain 
 * Enhanced Input functions like BindAction 
 * taking a UInputAction. 
 * Those functions are exclusive subclass features 
 * of UEnhancedInputComponent."
 */
