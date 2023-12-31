// Copyright Epic Games, Inc. All Rights Reserved.

#include "NG_ThirdPersonCharacter.h"

#include "Camera/CameraShakeBase.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// ANG_ThirdPersonCharacter

ANG_ThirdPersonCharacter::ANG_ThirdPersonCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(26.f, 92.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	bSprint = false;

	FOVCurrentValue = 90.f;
	FOVTargetValue = 90.f;
	InterpolationSpeed = 5.0f;
	// !!! causes the editor to crash
	//static ConstructorHelpers::FClassFinder<UUserWidget> SpeedWidgetBPClass(TEXT("/Game/Blueprints/WBP_SpeedInfo"));
	//if (SpeedWidgetBPClass.Class != NULL)
	//{
	//	SpeedWidget = SpeedWidgetBPClass.Class;
	//}
	
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ANG_ThirdPersonCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpeedWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), SpeedWidget);
	SpeedWidgetInstance->AddToViewport();  
}

void ANG_ThirdPersonCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	FOVCurrentValue = FMath::FInterpTo(FOVCurrentValue, FOVTargetValue, GetWorld()->GetDeltaSeconds(), InterpolationSpeed);
	FollowCamera->SetFieldOfView(FOVCurrentValue);
}

//////////////////////////////////////////////////////////////////////////
// Input

void ANG_ThirdPersonCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ANG_ThirdPersonCharacter::Sprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ANG_ThirdPersonCharacter::StopSprinting);
	PlayerInputComponent->BindAction("Sit", IE_Pressed, this, &ANG_ThirdPersonCharacter::Sit);
	PlayerInputComponent->BindAction("Sit", IE_Released, this, &ANG_ThirdPersonCharacter::StopSitting);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ANG_ThirdPersonCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ANG_ThirdPersonCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &ANG_ThirdPersonCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &ANG_ThirdPersonCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ANG_ThirdPersonCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ANG_ThirdPersonCharacter::TouchStopped);
}


void ANG_ThirdPersonCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void ANG_ThirdPersonCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ANG_ThirdPersonCharacter::Sprint()
{
	if (!bSprint)
	{
		OffAllState();
		bSprint = true;
		GetCharacterMovement()->MaxWalkSpeed *= 2.f;
		FOVTargetValue = 120.f;
	}
}

void ANG_ThirdPersonCharacter::StopSprinting()
{
	if (bSprint)
	{
		bSprint = false;
		GetCharacterMovement()->MaxWalkSpeed /= 2.f;
		FOVTargetValue = 90.f;
	}
}

void ANG_ThirdPersonCharacter::Sit()
{
	if (!bSit)
	{
		OffAllState();
		bSit = true;
		GetCharacterMovement()->MaxWalkSpeed /= 2.f;
	}
}

void ANG_ThirdPersonCharacter::StopSitting()
{
	if (bSit)
	{
		bSit = false;
		GetCharacterMovement()->MaxWalkSpeed *= 2.f;
	}
}

void ANG_ThirdPersonCharacter::OffAllState()
{
	bSprint = false;
	bSit = false;
}

void ANG_ThirdPersonCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ANG_ThirdPersonCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ANG_ThirdPersonCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		AddMovementInput(Direction, Value);
	}
}

void ANG_ThirdPersonCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}