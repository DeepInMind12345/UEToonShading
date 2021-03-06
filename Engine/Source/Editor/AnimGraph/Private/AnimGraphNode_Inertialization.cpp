// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphNode_Inertialization.h"

#include "EditorCategoryUtils.h"

#define LOCTEXT_NAMESPACE "AnimGraphNode_Inertialization"


FLinearColor UAnimGraphNode_Inertialization::GetNodeTitleColor() const
{
	return FLinearColor(0.0f, 0.1f, 0.2f);
}

FText UAnimGraphNode_Inertialization::GetTooltipText() const
{
	return LOCTEXT("NodeToolTip", "Inertialization");
}

FText UAnimGraphNode_Inertialization::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Inertialization");
}

FText UAnimGraphNode_Inertialization::GetMenuCategory() const
{
	return LOCTEXT("NodeCategory", "Inertialization");
}


#undef LOCTEXT_NAMESPACE
