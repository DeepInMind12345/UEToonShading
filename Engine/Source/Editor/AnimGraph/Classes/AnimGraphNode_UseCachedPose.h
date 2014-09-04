// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_Base.h"
#include "Animation/AnimNode_UseCachedPose.h"
#include "EdGraph/EdGraphNodeUtils.h" // for FNodeTextCache
#include "AnimGraphNode_UseCachedPose.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_UseCachedPose : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_UseCachedPose Node;

	UPROPERTY(EditAnywhere, Category=CachedPose)
	FString NameOfCache;

public:
	// UEdGraphNode interface
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual FString GetNodeCategory() const override;
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	// End of UAnimGraphNode_Base interface

private:
	/** Constructing FText strings can be costly, so we cache the node's title */
	FNodeTextCache CachedNodeTitle;
};
