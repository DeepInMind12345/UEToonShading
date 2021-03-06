// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ScreenPass.h"
#include "PostProcess/RenderingCompositionGraph.h"

BEGIN_SHADER_PARAMETER_STRUCT(FColorRemapParameters, )
	SHADER_PARAMETER(FVector, MappingPolynomial)
END_SHADER_PARAMETER_STRUCT()

FColorRemapParameters GetColorRemapParameters();

bool PipelineVolumeTextureLUTSupportGuaranteedAtRuntime(EShaderPlatform Platform);

FRDGTextureRef AddCombineLUTPass(FRDGBuilder& GraphBuilder, const FViewInfo& View);

FRenderingCompositeOutputRef AddCombineLUTPass(FRenderingCompositionGraph& Graph);