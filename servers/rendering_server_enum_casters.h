#pragma once

#include "servers/rendering_server_enums.h"
#include "core/method_enum_caster.h"
#include "core/string_utils.h"

// make variant understand the enums
VARIANT_NS_ENUM_CAST(RenderingServerEnums,CubeMapSide);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,TextureFlags);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ShaderMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ArrayType);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ArrayFormat);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,PrimitiveType);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,BlendShapeMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,LightType);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,LightParam);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ViewportUpdateMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ViewportClearMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ViewportMSAA);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ViewportUsage);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ViewportRenderInfo);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ViewportDebugDraw);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ScenarioDebugMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,InstanceType);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,NinePatchAxisMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,CanvasLightMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,CanvasLightShadowFilter);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,CanvasOccluderPolygonCullMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,RenderInfo);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,Features);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,MultimeshTransformFormat);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,MultimeshColorFormat);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,MultimeshCustomDataFormat);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,LightOmniShadowMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,LightOmniShadowDetail);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,LightDirectionalShadowMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,LightDirectionalShadowDepthRangeMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,LightBakeMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ReflectionProbeUpdateMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ParticlesDrawOrder);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,EnvironmentBG);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,EnvironmentDOFBlurQuality);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,EnvironmentGlowBlendMode);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,EnvironmentToneMapper);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,EnvironmentSSAOQuality);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,EnvironmentSSAOBlur);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,InstanceFlags);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ShadowCastingSetting);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,TextureType);
VARIANT_NS_ENUM_CAST(RenderingServerEnums,ChangedPriority);
